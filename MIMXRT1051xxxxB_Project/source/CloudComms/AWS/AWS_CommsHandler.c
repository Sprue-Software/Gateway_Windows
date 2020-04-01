
/*!****************************************************************************
*
* \file AWS_CommsHandler.c
*
* \author Carl Pickering
*
* \brief A shim to allow for the AWS communications to be abstracted out.
* It is anticipated this shim will be replaced with different shims if we wish
* to use other protocols such as COAP or alternative platforms such as Azure.
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
//nishi
#if 0
#include <aws_iot_config.h>
#include <aws_iot_version.h>
#include <aws_iot_json_utils.h>

#include <aws_iot_shadow_records.h>
#endif
#include "LOG_Api.h"
#include "C:/gateway/SA3075-P0302_2100_AC_Gateway/Port/Source/DeviceHandlers/Common/KVP_Api.h"
#include "OSAL_Api.h"
#include "LSD_Api.h"
#include "HAL.h"
#include "LSD_Types.h"
#include "LSD_EnsoObjectStore.h"
#include "AWS_CommsHandler.h"
#include "AWS_FaultBuffer.h"
#include "AWS_Timestamps.h"
#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "EnsoConfig.h"
#include "SYS_Gateway.h"
#include "APP_Types.h"
#include "C:/gateway/SA3075-P0302_2100_AC_Gateway/Port/Source/Storage/STO_Manager.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/

#define AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER (1000)

#define AWS_MAX_LENGTH_OF_LAST_WILL_MESSAGE    80

/* Sequencer Queue Size */
#define AWS_SEQ_QUEUE_CAPACITY              (50)

/* Maximum allowedPoll Request count in Queue. */
#define AWS_SEQ_QUEUE_MAX_POLL_COUNT        (AWS_SEQ_QUEUE_CAPACITY / 5)

#define AWS_CONNECTION_INITIAL_CONNECT_TIMEOUT        1        // Timeout for initial(immediate) connect
#define AWS_CONNECTION_MIN_RECONNECT_WAIT_INTERVAL_MS 1000     // Minimum time before the First reconnect attempt is made as part of the exponential back-off algorithm
#define AWS_CONNECTION_MAX_RECONNECT_WAIT_INTERVAL_MS 32000    // Minimum time before the First reconnect attempt is made as part of the exponential back-off algorithm

/*
 * For multiple connections, we add connection ID suffix _XX to thing name so
 * we need three byte more for client ID
 */
#define MQTT_CLIENT_NAME_BUFFER_SIZE        (ENSO_OBJECT_NAME_BUFFER_SIZE + 3)

/* Disconnect retry count */
#define AWS_DISCONNECT_RETRY_COUNT          3

/* Unsubscription retry count. -1 indicates the topic is still required */
#define AWS_UNSUBSCRIPTION_RETRY_COUNT      3
#define SUBSCRIPTION_ACTIVE                -1

/* Maximum times a yield to the aws library should be allowed to fail
 * before disconnecting and reconnecting the MQTT channel.
 */
#define AWS_YIELD_RETRY_COUNT 10

/* AWS Ping Interval */
#define AWS_PING_INTERVAL_IN_SEC      (60)

/* This constant is a flag used to indicate a JSON element is not a delta, ie. "no change". */
#define JSMN_NO_CHANGE (1 << 7)

/******************************************************************************
 * Type Definitions
 *****************************************************************************/

/**
 * \name EnsoAWSCLD_CommsChannel_t
 *
 * \brief A data structure used to handle the AWS Comms Channel
 *
 * It's far from ideal but as it's C instead of C++ implementing an
 * inheritance style interface using aggregation. It is desperately
 * important that the channelFunctionPointers remain the first items
 * in the structure.
 *
 */
typedef struct
{
    CLD_CommsChannel_t commsChannelBase;

//    ShadowInitParameters_t shadowParameters;
  //  AWS_IoT_Client mqttClient;
    char jsonDocumentBuffer[AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
    uint16_t numJsonEntriesInBuffer;

    /* Channel (MQTT Connection) Specific Thing Name. */
    char clientName[MQTT_CLIENT_NAME_BUFFER_SIZE];

    /* Channel specific Shadow RX Buffer */
    //char shadowRxBuf[SHADOW_MAX_SIZE_OF_RX_BUFFER];

    //jsmntok_t jsonTokenStruct[MAX_JSON_TOKEN_EXPECTED];

    /* Subscription count in channel */
    int32_t subscriptionCount;

    /* Update Delta Topics in channel */
    //char awsShadowUpdateDeltaTopic[MAX_SUBSCRIPTION_PER_MQTT_CHANNEL][MAX_SHADOW_TOPIC_LENGTH_BYTES];

    /* Number of delta topic unsubscription attempts or -1 if unsubscription not in progress */
    int8_t awsShadowUpdateDeltaTopicUnsubAttempts[MAX_SUBSCRIPTION_PER_MQTT_CHANNEL];

    /* Indicates whether Channel is connected */
    bool bConnected;

    /*
     * Indicates whether Channel is Enabled before.
     * When a connection is established before and if we lost or disconnect
     * connection, we can basically try to reconnect instead of new connection.
     * This flag is added to differentiate whether a connection is enabled before.
     */
    bool bEnabled;

    /* Channel ID */
    int32_t id;

    int32_t disconnectRetryCount;

    /* Channel specific connect timer */
    Timer_t connectTimer;
} AWS_CLD_CommsChannel_t;


typedef struct _deleteQueue_tag
{
    EnsoDeviceId_t deviceToDelete;
    struct _deleteQueue_tag* next;
} _deleteQueue_t;

/* Item Types for Sequencer */
typedef enum
{
    /* Channel Pollung */
    SeqItem_Poll,
    /* Delta Send */
    SeqItem_Delta,
    /* Subscribe to topic */
    SeqItem_Subscribe,
    /* Connect channel */
    SeqItem_Connect,
    /*Disconnect channel */
    SeqItem_Disconnect
} SeqItemType;

/*
 * Sequence Item Priorities
 *
 * IMP : Please be careful when you change this enumeration because below
 * implementation uses these values statically.
 */
typedef enum
{
    SeqItemPri_High,
    SeqItemPri_Low,
    SeqItemPri_Count,
} SeqItemPriority;

/*
 *  Sequencer Item Arguments
 */
typedef union
{
    struct
    {
        uint32_t channelID;
    } Poll;
    struct
    {
        uint32_t channelID;
        uint32_t slotIndex;
    } Subscribe;
    struct
    {
        uint32_t channelID;
    } Connect;
} SeqItemArgs;

/* Sequencer Item  */
typedef struct
{
    SeqItemType type;
    SeqItemArgs args;
} SeqQueueItem;

/*
 * Sequencer Queue Type.
 * A circular buffer is used to
 *  - reduce memory footprint (no need memory to link items)
 *  - Constant time - O(1) - code execution
 */
typedef struct
{
    int32_t head;   /* Head of queue */
    int32_t tail;   /* Tail of queue */
    int32_t count;  /* Item count in Queue */
    SeqQueueItem circularBuffer[AWS_SEQ_QUEUE_CAPACITY];
} SeqCircularQueue;


/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

/**
 * \brief The object to represent the AWS Comms Channel
 */
AWS_CLD_CommsChannel_t _awsCommsChannels[MAX_CONNECTION_PER_GATEWAY];

/* Internal Threads */
static Thread_t listeningThread;
static Thread_t sequencerThread;
static Thread_t deletingThread;
static Thread_t connectTimeoutThread;
static Thread_t outOfSyncThread;

/* Common Message Queue for all channels */
static MessageQueue_t _messageQueue;
static MessageQueue_t _deleteMessageQueue;
static MessageQueue_t _connectTimeoutMessageQueue;

static Mutex_t _commsMutex;
static Mutex_t _timerMutex;
static Mutex_t _deleteMutex;
static Mutex_t _seqMutex;

static Semaphore_t _outOfSyncSemaphore;

static char _gatewayThingName[ENSO_OBJECT_NAME_BUFFER_SIZE];

/* Connection Parameters */
//static IoT_Client_Connect_Params _connectParams;

/*
 * Connection Retry Timeout Value.
 * We are using Exponential back-off so we multiply timeout value by two
 * by starting AWS_MIN_RECONNECT_WAIT_INTERVAL
 */
static int32_t _connRetryTimeout = AWS_CONNECTION_MIN_RECONNECT_WAIT_INTERVAL_MS;

/* Yield failure counts
 * Count of the number of failures when yielding to AWS so we don't keep trying
 * for ever if a MQTT connection becomes locked up. */
static int8_t yieldFailureCount[MAX_CONNECTION_PER_GATEWAY];

// The aws-iot-mqtt library only keeps pointers to topic names so allocate
// memory for the topic names.
//char devAnnTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];
//char devDelTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];
//char devAnnAcceptTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];
//char devAnnRejectTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];
//char devDeleteAcceptTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];
//char devCancelTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];
//char lastWillTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];
//char lastWillMessage[AWS_MAX_LENGTH_OF_LAST_WILL_MESSAGE];

static EnsoDeviceId_t _gatewayId;

static uint32_t _commsDelayMs = 0;

/* Flag to indicate whether AWS Module is started */
static bool _bAWSStarted = false;

/* Timer status */
static bool _bOutOfSyncTimerEnabled = false;
static Timer_t outOfSyncShadowTimer = NULL;
static Timer_t _AwsPollingTimer = NULL;
static Thread_t _AwsPollingThread;  //////// [RE:workaround] We use a thread instead of the _AwsPollingTimer timer
static Timer_t _deleteTimer = NULL;


/*
 * Flag to indicate whether device (gateway) subscribed to Accept and Reject
 * Topics to avoid duplicate subscription.
 * It also blocks objects to subscribe topics if gateway not subscribed to
 * Accept/Reject topics yet
 */
static bool _bGatewayAcceptRejectTopicsSubscribed = false;

/* Flag to indicate whether GW Announce is accepted */
static bool _bGatewayAnnounceAccepted = false;

/* Check for could not be unsubscribed objects */
static bool _bCheckForNotUnsubscribedObjects = false;

/* Flag to indicate Gateway cloud registration status */
static bool _bGateWayRegistered = false;

/* Flag to indicate whether we have a communication in progress */
static bool _bUpdateInProgress = false;

static uint32_t min_ms_between_deltas = 0;

/*
 * AWS Polling Interval.
 * We are using more than one connection so we need to poll all of them
 * sequentially. This timer timeout indicates period of polling.
 */
static uint32_t aws_polling_interval_in_ms = 0;
/*
 * AWS Yield timeout.
 * Once a channel is enabled, we need to yield to MQTT connection.
 */
static uint32_t aws_polling_yield_timeout_in_ms = 0;

static uint32_t aws_out_of_sync_initial_shadow_recovery_interval_ms = 0;
static uint32_t aws_out_of_sync_short_shadow_recovery_interval_ms = 0;

static _deleteQueue_t* _deleteQueue;

/*
 * AWS Sequencer Queues
 * We are creating seperate queues for different priorities.
 * We could use a priority queue (heap) to manage all items in single queue.
 */
static SeqCircularQueue _awsSeqQueues[SeqItemPri_Count];

/* Counter to keep Poll Request count in AWS Sequencer */
int _awsSeqWaitingPollCount = 0;

/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static void _AWS_MessageQueueListener(MessageQueue_t mq);
#if 0
static void _AWS_CommsChannelCallback(
        const char *pThingName,
        //ShadowActions_t action,
        //Shadow_Ack_Status_t status,
        const char *pReceivedJsonDocument,
        void *pContextDat);

static bool _AWS_IsJsonKeyMatching(
        const char *pJsonDocument,
        void *pJsonHandler,
        int32_t tokenCount,
        const char *pKey,
        uint32_t *pDataIndex);

static IoT_Error_t _AWS_ParsePrimitive(
        EnsoValueType_e type,
        EnsoPropertyValue_u *pValue,
        const char *pJsonString,
        jsmntok_t token);

void _AWS_ShadowDeltaCallback(
        AWS_IoT_Client *pClient,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData);

static EnsoErrorCode_e _AWS_UpdateThingStatus(
        const char *thingName,
        EnsoDeviceStatus_e status);

void _AWS_AnnounceAccept(
        AWS_IoT_Client *unused,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData);

void _AWS_AnnounceReject(
        AWS_IoT_Client *pClient,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData);

static void _AWS_GWDeleteAccept(
        AWS_IoT_Client *pClient,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData);

static void _AWS_CancelAccept(
        AWS_IoT_Client *pClient,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData);

static void _AWS_IOT_SendOutOfSyncShadowsCB(void *handler);
static void _AWS_IOT_SendOutOfSyncShadows(void *handler);

static void _AWS_PollingPass(void * handle);

static void _AWS_DisconnectCallbackHandler(AWS_IoT_Client *pClient, void *data);

static void _AWS_IOT_ConnectTimerCB(void* handle);

static void _AWS_ConnectChannel(uint32_t channelID);

static EnsoErrorCode_e _AWS_ParseJsonValue(
        char* shadowRxBuf,
        jsmntok_t* jsonTokenStruct,
        EnsoDeviceId_t* deviceId,
        const char * propName,
        int jsonTokenIndex);

static char * _AWS_Strerror(IoT_Error_t error);

static EnsoErrorCode_e AWS_IOT_ShadowConnect(
        AWS_CLD_CommsChannel_t *channel,
        EnsoDeviceId_t* gatewayId);

static EnsoErrorCode_e _AWS_ParseStringValue(
        char *buf, int bufSize,
        const char *jsonString,
        jsmntok_t *token);

void _AWS_ClearOutOfSyncFlag(
        const char* thingName,
        const char* jsonResponse);

static int _AWS_CheckKeyValuePairs(
        const char * thingName,
        const char * propParentName,
        const char * json,
        const int indexStart,
        const int positionEnd,
        const int tokenCount,
        jsmntok_t* jsonTokenStruct);

EnsoErrorCode_e _AWS_VerifyPropertyValue(
        const char * thingName,
        const char * propName,
        const char * json,
        jsmntok_t token);
#endif
EnsoErrorCode_e _AWS_GenerateLastWillAndTestament(void);

static void _AWS_ObjectDeleteWorker(MessageQueue_t mq);

static void _AWS_SetTimerForDeleteWorker(void);

static void _AWS_KickDeleteWorker(void* unused);

static void _AWS_AddToDeleteQueue(EnsoDeviceId_t device);

static void _AWS_RemoveFromDeleteQueue(char* deviceName);

static bool _AWS_GatewayAnnounceAndSubscriptions(char* parentName, EnsoObject_t* owner);

static void _AWS_AnnounceNewThing(EnsoObject_t* owner, EnsoDeviceId_t* deviceID, char* parentName, char* thingName,
                                  bool isGateway, uint32_t thingType, int32_t connectionID);

/******************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * Sends LED Status(e.g. AWS Connection Status) to LED Handler
 *
 * \param propID LED Property ID
 * \param ledState
 *
 * \return EnsoError Code
 */
EnsoErrorCode_e _AWS_SendLEDStatus(EnsoAgentSidePropertyId_t propId, bool ledState)
{
    EnsoDeviceId_t deviceId;
    SYS_GetDeviceId(&deviceId);

    EnsoPropertyValue_u newValue = {
            .booleanValue = ledState
    };

    /* Create a delta */
    EnsoPropertyDelta_t delta =
    {
            .agentSidePropertyID = propId,
            .propertyValue = newValue
    };

    LOG_Info("Sending update to led_device_handler - ledState: %s\n", ((ledState) ? "True" : "False"));

    /* Send delta to message queue */
    EnsoErrorCode_e retVal = ECOM_SendUpdateToSubscriber(LED_DEVICE_HANDLER,
                                                         deviceId,
                                                         REPORTED_GROUP,
                                                         1,
                                                         &delta);

    return retVal;
}

/**
 * \name _AWS_GetConnectionIDofDevice
 *
 * \brief Returns Connection ID of device
 *
 * \param  deviceID     Device ID
 * \param[out]  connectionID Connection ID of device
 *
 * \return EnsoErrorCode_e
 */
static EnsoErrorCode_e _AWS_GetConnectionIDofDevice(const EnsoDeviceId_t* deviceID, int32_t* connectionID)
{
    EnsoErrorCode_e retVal = eecNoError;

    /* Read Connection and Subscription ID from Local Shadow */
    EnsoPropertyValue_u connVal = { 0 };
    retVal = LSD_GetPropertyValueByAgentSideId(
                deviceID,
                REPORTED_GROUP,
                PROP_CONNECTION_ID,
                &connVal);

    if (eecNoError != retVal)
    {
        LOG_Error("LSD_GetPropertyValueByAgentSideId err %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }

    *connectionID = connVal.int32Value;

    if (*connectionID >= MAX_CONNECTION_PER_GATEWAY)
    {
        LOG_Error("Invalid Connection ID %d ", *connectionID);
        return eecParameterOutOfRange;
    }

    return retVal;
}

/**
 * \name _AWS_GetConnectionIDForThingName
 *
 * \brief Returns Connection ID of specified Thing Name
 *
 * \param  thingName    Thing Name
 * \param[out]  connectionID Connection ID of device
 *
 * \return EnsoErrorCode_e
 */
static EnsoErrorCode_e _AWS_GetConnectionIDForThingName(const char* thingName, int32_t* connectionID)
{
    EnsoDeviceId_t deviceID;
    EnsoErrorCode_e retVal = LSD_GetThingFromNameString(thingName, &deviceID);

    if (eecNoError != retVal)
    {
        LOG_Error("LSD_GetThingFromNameString failed %d", retVal);
        return retVal;
    }

    return _AWS_GetConnectionIDofDevice(&deviceID, connectionID);
}

/**
 * \name _AWS_FindAvailableConnectionForDevice
 *
 * \brief Finds available connection(channel) for new device
 *
 * \param[out]  availableConnectionNo Available connection ID
 * \param[out]  bNewChannel Is it new channel (Not connected to server yet)
 *
 * \return bool Returns true if an available slot exists for new device
 */
static bool _AWS_FindAvailableConnectionForDevice(int32_t* availableConnectionNo, bool* bNewChannel)
{
    bool retVal = false;

    *bNewChannel = false;
    *availableConnectionNo = -1;

    for (int32_t i = 0; i < MAX_CONNECTION_PER_GATEWAY; i++)
    {
        int32_t subscriptionCount = _awsCommsChannels[i].subscriptionCount;

        if (subscriptionCount < MAX_SUBSCRIPTION_PER_MQTT_CHANNEL)
        {
            retVal = true;

            /* If there is no subscription in channel, it means it is a new channel */
            if (subscriptionCount == 0)
            {
                *bNewChannel = true;
            }

            /* Set available connection */
            *availableConnectionNo = i;

            break;
        }
    }

    return retVal;
}

/**
 * \name _AWS_FindDeltaTopicIndexByThingName
 *
 * \brief Finds index of delta topic of thing using thing name.
 *
 * \param      deviceThingName Thing name to search delta topic or NULL for any topic awaiting unsubscription
 * \param[out] connectionID Connection ID of thing
 *
 * \return Index of delta topic in case of success. Otherwise, returns -1
 */
static int32_t _AWS_FindDeltaTopicIndexByThingName(const char* deviceThingName, int32_t* connectionID)
{
#if 0
    *connectionID = -1;

    for (int32_t channelIndex = 0; channelIndex < MAX_CONNECTION_PER_GATEWAY; channelIndex++)
    {
        AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[channelIndex];

        for (int32_t index = 0; index < MAX_SUBSCRIPTION_PER_MQTT_CHANNEL; index++)
        {
//            char* deltaTopic = channel->awsShadowUpdateDeltaTopic[index];

            /* Ignore if no string in buffer */
            if (strlen(deltaTopic) == 0)
            {
                continue;
            }
            if (deviceThingName != NULL)
            {
                char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];
                /* Get thing name from Delta topic */
                int parsed;
                if (deltaTopic[36] == '_')
                {
                    parsed = sscanf(deltaTopic, "$aws/things/%49[^'/']/%*s", thingName);
                }
                else
                {
                    parsed = sscanf(deltaTopic, "$aws/things/%24[^'/']/%*s", thingName);
                }
                if (parsed != 1)
                {
                    LOG_Error("sscanf error for %s", deltaTopic);
                    continue;
                }

                /* Compare thing names */
                if (strncmp(deviceThingName, thingName, ENSO_OBJECT_NAME_BUFFER_SIZE) == 0)
                {
                    *connectionID = channelIndex;

                    return index;
                }
            }
            else if ( channel->awsShadowUpdateDeltaTopicUnsubAttempts[index] > 0)
            {
                // Found any topic awaiting unsubscription
                *connectionID = channelIndex;
                return index;
            }
        }
    }
#endif
    return -1;
}

/**
 * \name _AWS_FindDeltaTopicIndexByDeviceId
 *
 * \brief Finds index of delta topic of thing using thing id.
 *
 * \param      deviceID Thing id to search delta topic
 * \param[out] connectionID Connection ID of thing
 *
 * \return Index of delta topic in case of success. Otherwise, returns -1
 */
static int32_t _AWS_FindDeltaTopicIndexByDeviceId(const EnsoDeviceId_t* deviceID, int32_t* connectionID)
{
    int bufferUsed;
    char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];
    EnsoErrorCode_e retVal = LSD_GetThingName(thingName, sizeof(thingName), &bufferUsed, *deviceID, _gatewayId);
    if (eecNoError != retVal)
    {
        LOG_Error("Failed to get Thing Name ", LSD_EnsoErrorCode_eToString(retVal));
        return -1;
    }

    /* We need to find delta topic index of to be unsubscribed device */
    int32_t deltaTopicIndex = _AWS_FindDeltaTopicIndexByThingName(thingName, connectionID);
    if (deltaTopicIndex < 0 || deltaTopicIndex >= MAX_SUBSCRIPTION_PER_MQTT_CHANNEL)
    {
        LOG_Error("Delta Topic for unsubscribed device (%s) does not exist", thingName);

        return -1;
    }
    return deltaTopicIndex;

}

/**
 * \name _AWS_FindEmptySlotForDeltaTopic
 *
 * \brief Finds and empty slot for Delta Topic for a new device and returns
 *        slot index
 *
 * \param  channel Channel to search empty slot
 *
 * \return Index of empty slot in case of success. Otherwise, returns -1
 */
static int32_t _AWS_FindEmptySlotForDeltaTopic(AWS_CLD_CommsChannel_t* channel)
{
    /*
     * Index to start to search for an empty slot.
     * For channel 0, first two slot is occupied by Announce Accept and Reject
     * topics so ignore them
     */
    int32_t startIndex = (channel->id == 0) ? 2 : 0;

    for (int32_t i = startIndex; i < MAX_SUBSCRIPTION_PER_MQTT_CHANNEL; i++)
    {
        if (1)
        {
            channel->awsShadowUpdateDeltaTopicUnsubAttempts[i] = SUBSCRIPTION_ACTIVE;
            return i;
        }
    }

    return -1;
}

/**
 * \name _AWS_QueueSeqItem
 *
 * \brief Push new item to AWS Sequencer Queue
 *
 * \param  item to add queue
 *
 * \return true if able to queue new item to sequencer queue otherwise returns false
 */
static bool _AWS_QueueSeqItem(SeqQueueItem item, SeqItemPriority priority)
{
    bool retVal = false;

    OSAL_LockMutex(&_seqMutex);

    /* Queue according to item priority */
    SeqCircularQueue* queue = &_awsSeqQueues[priority];

    if (item.type == SeqItem_Poll)
    {
        /*
         * Additional Polls do not make sense so if there is enough poll request
         * in AWS Sequencer Queue, we can ignore them.
         */
        if (_awsSeqWaitingPollCount > AWS_SEQ_QUEUE_MAX_POLL_COUNT)
        {
            OSAL_UnLockMutex(&_seqMutex);
            /* It is not a failure */
            return true;
        }

        /* We need to track waiting Poll Count */
        _awsSeqWaitingPollCount++;
    }

    /* Queue new item */
    if (queue->count < AWS_SEQ_QUEUE_CAPACITY)
    {
        queue->circularBuffer[queue->head] = item;

        queue->head = (queue->head + 1) % AWS_SEQ_QUEUE_CAPACITY;
        queue->count++;

        retVal = true;
    }

    OSAL_UnLockMutex(&_seqMutex);

    return retVal;
}

/**
 * \name _AWS_EnqueueSeqItem
 *
 * \brief Enqueues item from AWS Sequencer Queue
 *
 * \param[out] enqueued item
 *
 * \return true if able to enqueue new item from sequencer queue otherwise returns false
 */
static bool _AWS_EnqueueSeqItem(SeqQueueItem* item)
{
    bool retVal = false;

    OSAL_LockMutex(&_seqMutex);

    /* Start from high priority queues to low priority */
    for (int qIndex = 0; qIndex < SeqItemPri_Count; qIndex++)
    {
        SeqCircularQueue* queue = &_awsSeqQueues[qIndex];

        /* Test queue */
        if (queue->count > 0)
        {
            *item = queue->circularBuffer[queue->tail];

            queue->tail = (queue->tail + 1) % AWS_SEQ_QUEUE_CAPACITY;
            queue->count--;

            retVal = true;

            break;
        }
    }

    OSAL_UnLockMutex(&_seqMutex);

    return retVal;
}

/**
 * \name _AWS_SubscribeToChildTopic
 *
 * \brief Subscribes device to child delta topic
 *
 * \param channelID Channel ID for subscription
 * \param topicSlotIndex Slot Index to get Subscription Topic
 *
 * \return none
 */
static void _AWS_SubscribeToChildTopic(uint32_t channelID, uint32_t topicSlotIndex)
{
    AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[channelID];
  //  char* awsShadowUpdateDeltaTopic = channel->awsShadowUpdateDeltaTopic[topicSlotIndex];
    char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];

    /* Parse Thing name from delta topic  */
    {
      //  char* thingStartPtr = strstr(awsShadowUpdateDeltaTopic, "things/");
        //char* thingEndPtr = strstr(awsShadowUpdateDeltaTopic, "/shadow");
       // if (thingStartPtr == NULL || thingEndPtr == NULL)
        if (1)
        {
           // LOG_Warning("Invalid topic name %s. Failed to subscribe new device! ", awsShadowUpdateDeltaTopic);
            return;
        }

        //thingStartPtr += strlen("things/");
        //int32_t thingLen = (thingEndPtr - thingStartPtr);

       // memcpy(thingName, thingStartPtr, thingLen);
       // thingName[thingLen] = '\0';
    }

    OSAL_LockMutex(&_commsMutex);
    // Note that the following call is blocking
#if 0
    IoT_Error_t rc = aws_iot_mqtt_subscribe(
            &channel->mqttClient,
            awsShadowUpdateDeltaTopic,
            (uint16_t) strlen(awsShadowUpdateDeltaTopic),
            QOS0,
            _AWS_ShadowDeltaCallback,
            NULL);
#endif
    OSAL_UnLockMutex(&_commsMutex);

    if (1)
    {
  //      LOG_Error("aws_iot_mqtt_subscribe failed 3 %d", rc);

        /*
         * Subscription count was increased and will be increased for the next
         * time so we need to decrease it.
         */
        channel->subscriptionCount--;

        /* Clean update delta topic for failed subscription.  */
     //   memset(awsShadowUpdateDeltaTopic, 0, MAX_SHADOW_TOPIC_LENGTH_BYTES);
    }
    else
    {
    //    LOG_Info("Subscribed to %s", awsShadowUpdateDeltaTopic);
    }
}

/**
 * \name _AWS_Sequencer
 *
 * \brief Executes AWS Requests sequentially
 *
 * \param args not used
 *
 * \return none
 */
static void _AWS_Sequencer(void* args)
{
    SeqQueueItem item;
item.args.Poll.channelID = 0;
    while (1)
    {
        /* Enqueue an item from AWS Sequencer Queue  */
        bool bExist = _AWS_EnqueueSeqItem(&item);

        if (bExist)
        {
            switch(item.type)
            {
                case SeqItem_Delta:
                    /* Trigger Fault Buffer to send next waiting delta */
                    AWS_SendWaitingDelta();
                    break;
                case SeqItem_Poll:
                    {
                        /* We need to track poll count in Sequencer Queue */
                        _awsSeqWaitingPollCount--;

                        AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[item.args.Poll.channelID];

                        /* Poll Channel if still connected */
                        if (channel->bConnected)
                        {
                            AWS_CommsChannelPoll((CLD_CommsChannel_t*)channel);
                        }
                    }
                    break;
                case SeqItem_Subscribe:
                    /*
                     * Subscribe to child topic
                     * We could subscribe in Announce Accept Callback but
                     * this callback called by Poll method and Poll methods
                     * need to be smaller for fair channel scheduling between
                     * channels.
                     * In case of subscription timeout, it takes 20 seconds
                     * which means poll method will take 20 seconds too.
                     * To avoid this issue, lets queue subscription request
                     * instead of calling immediately in announce accept.
                     */
                    _AWS_SubscribeToChildTopic(item.args.Subscribe.channelID, item.args.Subscribe.slotIndex);
                    break;
                case SeqItem_Connect:
                   // _AWS_ConnectChannel(item.args.Connect.channelID);
                    break;
                case SeqItem_Disconnect:
                    {
                        LOG_Info("Sequence Item Disconnect");
                        AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[item.args.Connect.channelID];
                        AWS_CommsChannelClose((CLD_CommsChannel_t*)channel);
                    }
                    break;
                default:
                    break;
            }
        }
        else
        {
            /* Sleep to reduce CPU utilisation if queue is empty */
            OSAL_sleep_ms(100);
        }
    }
}

/**
 * \name _AWS_UnsubscribeDevice
 *
 * \brief Unsubscribes device when it removed
 *
 * \param  deltaTopicIndex Index to delta topic being unsubscribed
 * \param  connectionID Connection id for the delta topic being unsubscribed
 *
 * \return EnsoErrorCode_e
 */
static EnsoErrorCode_e _AWS_UnsubscribeDevice(const int32_t deltaTopicIndex,
                                              const int32_t connectionID)
{
    /* In case an unsubscription fails, Comms Handler periodically retries to
     * unsubscribe */

    if (deltaTopicIndex < 0 || deltaTopicIndex >= MAX_SUBSCRIPTION_PER_MQTT_CHANNEL)
    {
        LOG_Error("Delta Topic for unsubscribed device does not exist");
        return eecInternalError;
    }

    AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[connectionID];
    char* awsShadowUpdateDeltaTopic = "resr" ;//channel->awsShadowUpdateDeltaTopic[deltaTopicIndex];
    LOG_Info("Delta topic = %s", awsShadowUpdateDeltaTopic);

    LOG_Info("Unsubscribe retry count: %i", channel->awsShadowUpdateDeltaTopicUnsubAttempts[deltaTopicIndex]);
    if ( SUBSCRIPTION_ACTIVE == channel->awsShadowUpdateDeltaTopicUnsubAttempts[deltaTopicIndex])
    {
        // First unsubscribe attempt. Set maximum retry count
        LOG_Info("First unsubscribe attempt. Max attempts: %i", AWS_UNSUBSCRIPTION_RETRY_COUNT);
        channel->awsShadowUpdateDeltaTopicUnsubAttempts[deltaTopicIndex] = AWS_UNSUBSCRIPTION_RETRY_COUNT;
    }

    if (channel->awsShadowUpdateDeltaTopicUnsubAttempts[deltaTopicIndex] <= 0)
    {
        /* No new retry for unsubscription */
        LOG_Warning("Unsubscribe retry limit reached %u", AWS_UNSUBSCRIPTION_RETRY_COUNT);
        /* Release delta topic. */
     //   memset(awsShadowUpdateDeltaTopic, 0, MAX_SHADOW_TOPIC_LENGTH_BYTES);
        channel->awsShadowUpdateDeltaTopicUnsubAttempts[deltaTopicIndex] = 0;
        return eecRemoveFailed;
    }
#if 0
    /* Get channel(connection) of device  */
    AWS_IoT_Client *pClient = &channel->mqttClient;

    /* Unsubscribe device from topic */
    IoT_Error_t rc = aws_iot_mqtt_unsubscribe(
                        pClient,
                        awsShadowUpdateDeltaTopic,
                        (uint16_t) strlen(awsShadowUpdateDeltaTopic));

    // If the MQTT connection isn't ready, try again later. If the unsubscribe
    // failed for any other reason (nothing else we can do for other errors) or
    // it was successful then delete the stored delta topic data.
    if (NETWORK_DISCONNECTED_ERROR == rc ||
        MQTT_CLIENT_NOT_IDLE_ERROR == rc)
    {
        LOG_Info("Waiting to unsubscribe the deleted thing. Err: %s",
                 _AWS_Strerror(rc));

        (channel->awsShadowUpdateDeltaTopicUnsubAttempts[deltaTopicIndex])--;

        /*
         * Unsubscription retries are handled together with out of sync
         * objects so set a timer if not enabled.
         */
        AWS_StartOutOfSyncTimer();

        return eecMQTTClientBusy;
    }
#endif
    /* Decrease subscription count of  */
    channel->subscriptionCount--;

    /* Release delta topic. */
 //   memset(awsShadowUpdateDeltaTopic, 0, MAX_SHADOW_TOPIC_LENGTH_BYTES);
    channel->awsShadowUpdateDeltaTopicUnsubAttempts[deltaTopicIndex] = 0;


    LOG_Info("Thing is unsubscribed!");

    /*
     * If there is no subscription in connection (except connection 0), we close
     * the connection
     */
    if ((channel->subscriptionCount == 0) && channel->id != 0)
    {
        LOG_Info("No subscription in connection. Connection %d is closing!", channel->id);

       // rc = aws_iot_mqtt_disconnect(&channel->mqttClient);

       // if (SUCCESS != rc)
        if (true)
        {
            LOG_Error("Failed to close connection %d", channel->id);

            channel->disconnectRetryCount = AWS_DISCONNECT_RETRY_COUNT;

            /*
             * Disconnect retries are handled together with out of sync
             * objects so set out of sync timer if not enabled.
             */
            AWS_StartOutOfSyncTimer();

            return eecRemoveFailed;
        }
        else
        {
            /* Do not set retry count in case of success */
            channel->disconnectRetryCount = 0;
        }
    }

    return eecNoError;
}


/**
 * \name _AWS_UnsubscribeDeviceID
 *
 * \brief Unsubscribes a delta topic based on device id
 *
 * \param  deviceID To be unsubscribed Device
 *
 * \return EnsoErrorCode_e
 */
static EnsoErrorCode_e _AWS_UnsubscribeDeviceID(const EnsoDeviceId_t* deviceID)
{
    EnsoErrorCode_e retVal;

    // What is the device name string?
    int bufferUsed;
    char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];
    retVal = LSD_GetThingName(thingName, sizeof(thingName), &bufferUsed, *deviceID, _gatewayId);
    if (eecNoError != retVal)
    {
        LOG_Error("Failed to get Thing Name ", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }

    /* We need to find delta topic index of to be unsubscribed device */
    int32_t connectionID;
    int32_t deltaTopicIndex = _AWS_FindDeltaTopicIndexByDeviceId(deviceID, &connectionID);
    _AWS_UnsubscribeDevice(deltaTopicIndex, connectionID);
    return eecNoError;
}

/**
 * Checks for failed disconnects
 *
 * \param finished All of disconnect attempt are success
 *
 * \return none
 */
void _AWS_CheckForDisconnectRetries(bool* finished)
{
    *finished = true;

    for (int i = 1; i < MAX_CONNECTION_PER_GATEWAY; i++)
    {
        AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[i];

        if (channel->disconnectRetryCount > 0)
        {
            /* We found a channel waits for disconnect so do not mark flag as not finished */
            *finished = false;

            LOG_Info("Retry to close connection %d", channel->id);

        //    IoT_Error_t rc = aws_iot_mqtt_disconnect(&channel->mqttClient);

            if (true)
            {
                LOG_Error("Failed to close connection %d", channel->id);

                channel->disconnectRetryCount--;
            }
            else
            {
                /* Clear retry count in case of success */
                channel->disconnectRetryCount = 0;
            }

            /*
             * If there exist any other channel waits for disconnect just wait
             * try for the next time.
             * */
            break;
        }
    }
}

/**
 * Checks for unsubscriptions which could not be completed before.
 *
 * \param finished All of unsubscriptions are completed
 *
 * \return none
 */
void _AWS_CheckForUnsubscriptions(bool* finished)
{
    /* Find delta topic index of any to-be-unsubscribed device */
    int32_t connectionID;
    int32_t deltaTopicIndex = _AWS_FindDeltaTopicIndexByThingName(NULL, &connectionID);

    if (deltaTopicIndex < 0)
    {
        *finished = true;
    }
    else
    {
        *finished = false;

        /* Retry to unsubscribe object */
        _AWS_UnsubscribeDevice(deltaTopicIndex, connectionID);
    }
}

/**
 * Indicate whether we are catching up with comms
 *
 * \param none
 *
 * \return none
 */
bool AWS_OutOfSyncTimerEnabled()
{
    return _bOutOfSyncTimerEnabled;
}

/**
 * Starts out of sync timer if it is not already running
 *
 * \param none
 *
 * \return none
 */
void AWS_StartOutOfSyncTimer(void)
{
    OSAL_LockMutex(&_timerMutex);

    if (!_bOutOfSyncTimerEnabled)
    {
        if (outOfSyncShadowTimer)
        {
            OSAL_DestroyTimer(outOfSyncShadowTimer);
            outOfSyncShadowTimer = NULL;
        }
      //  outOfSyncShadowTimer = OSAL_NewTimer(_AWS_IOT_SendOutOfSyncShadowsCB, _commsDelayMs, false, NULL);

        if (NULL == outOfSyncShadowTimer)
        {
            LOG_Error("Could not create Out-Of-Sync Shadow Recover timer.");
            _bOutOfSyncTimerEnabled = false;
        }
        else
        {
            _bOutOfSyncTimerEnabled = true;
        }
    }

    OSAL_UnLockMutex(&_timerMutex);
}

/**
 * \name _AWS_IOT_SendOutOfSyncShadowsCB
 *
 * \brief Send Out of Sync Local Shadow after connection is re-established.
 *
 * \param  handler - not used
 *
 * \return none
 *
 */
static void _AWS_IOT_SendOutOfSyncShadowsCB(void* handler)
{
    OSAL_GiveBinarySemaphore(&_outOfSyncSemaphore);
}


/**
 * \name _AWS_IOT_SendOutOfSyncShadows
 *
 * \brief Send Out of Sync Local Shadow after connection is re-established.
 *
 * \param  none
 *
 * \return none
 *
 */
static void _AWS_IOT_SendOutOfSyncShadows(void *handler)
{
    static int counter = 0;

    // For ever loop
    for ( ; ; )
    {
        OSAL_TakeBinarySemaphore(&_outOfSyncSemaphore);

        // We only get here from timer, we know timer has thus fired
        _bOutOfSyncTimerEnabled = false;

        if (_bUpdateInProgress)
        {
            // We must serialise updates
            counter++;

            AWS_StartOutOfSyncTimer();

            if (counter > 60)
            {
                // We've got stuck, callback not fired?
                _bUpdateInProgress = false;
                LOG_Error("Forced update in progress to false as we got stuck");
            }

            continue;
        }

        counter = 0;

        /*
         * All of shadow deltas are sent over connection 0 and connection 0 should
         * be up and running to send out of sync properties.
         * Otherwise, break to retry.
         * When connection 0 is re-established later, a new timer will be set to run
         * this function again.
         */
        if (!_awsCommsChannels[0].bConnected)
        {
            continue;
        }

        /*
         * There is no point sending properties until all announces are done
         */

        bool finished = false;

        /*
         * Check for only failed channel disconnects
         */
        _AWS_CheckForDisconnectRetries(&finished);

        if (finished)
        {
            /*
             * Check for only incomplete unsubscriptions.
             * AWS_CheckForUnsubscriptions()  function visits all objects so check
             * only an incomplete unsubcription exists.
             */
            if (_bCheckForNotUnsubscribedObjects)
            {
                _AWS_CheckForUnsubscriptions(&finished);
            }
            else
            {
                finished = true;
            }
        }

        if (finished)
        {
            /*
             * If incomplete unsubscriptions is finished, clear this flag to not
             * check them again.
             */
            _bCheckForNotUnsubscribedObjects = false;

            /* Check for unannounced objects */
            LSD_AnnounceEnsoObjects(&finished);
        }

        if (!finished)
        {
            AWS_StartOutOfSyncTimer();
        }
        else
        {
            // Can now process properties
            AWS_SetTimerForPeriodicWorker();
        }
    }
}

/**
 * \name AWS_SendECOMConnectMessage
 *
 * \brief Sends an ECOM_CONNECT message to the Comms handler to initiate connection to AWS
 *        on a channel.
 */
void AWS_SendECOMConnectMessage(void * handle)
{
    LOG_Trace("%p", handle);
    ECOM_ConnectMessage_t message = { .messageId = ECOM_CONNECT, .channel = handle };
    if (OSAL_SendMessage(_messageQueue, &message, sizeof message, MessagePriority_high) < 0)
    {
        LOG_Error("OSAL_SendMessage failed %s", strerror(errno));
    }
}
#if 0
static void _AWS_ConnectChannel(uint32_t channelID)
{

	/* Do not allow to connect if AWS not started or stopped! */
    if (!_bAWSStarted)
    {
        return;
    }

    AWS_CLD_CommsChannel_t *channel = &_awsCommsChannels[channelID];

    ////////IoT_Error_t rc;
  //  IoT_Error_t rc = FAILURE;   //////// [RE:fixed] Inited this
    // If we haven't got time yet, we won't be able to connect so don't try
    EnsoPropertyValue_u timeStatus = LSD_GetTimeNow();
    if (!timeStatus.timestamp.isValid)
    {
        LOG_Info("Time is not valid yet can't connect, retry in 30s");
        if (channel->connectTimer)
        {
            ////////// [RE:fixme] Is it safe to delete ourselves blocking-ly and while there's still code to run?
            OSAL_DestroyTimer(channel->connectTimer);
            channel->connectTimer = NULL;
        }
        channel->connectTimer = OSAL_NewTimer(_AWS_IOT_ConnectTimerCB, AWS_CONNECTION_MAX_RECONNECT_WAIT_INTERVAL_MS, false, channel);
        if (channel->connectTimer == NULL)
        {
            LOG_Error("Failed to set timer for connecting?")
        }
        return;
    }
    OSAL_LockMutex(&_commsMutex);
    /*
     * _connectParams are common for all channels but this function is called
     * asynchronously in case of connection fails for each channel so we need
     * to set following values for each connection retry.
     */
    //_connectParams.pClientID = channel->clientName;
    //_connectParams.clientIDLen = (uint16_t) strlen(_connectParams.pClientID);
    if (!channel->bEnabled)
    {
        /* Try to connect cloud */
    //    rc = aws_iot_mqtt_connect(&channel->mqttClient, &_connectParams);
    }
    else
    {
        /*
         * Existing channel was broken so instead of new connection, try
         * to reconnect.
         */

        // Regenerate last will and testament as sequence number has increased
        _AWS_GenerateLastWillAndTestament();
     //   rc = aws_iot_mqtt_attempt_reconnect(&channel->mqttClient);
    }
    OSAL_UnLockMutex(&_commsMutex);
   // if (NETWORK_RECONNECTED == rc || NETWORK_ALREADY_CONNECTED_ERROR == rc)
    if (1)
    {
        /*
         * Following code looks on for SUCCESS as return value so lets
         * replace rc with SUCCESS for following cases
         *  - reconnected
         *  - already connected
         */
        //rc = SUCCESS;
    }
    if (true)
    {
        LOG_Error("aws_iot_mqtt_connect failed %s", _AWS_Strerror(rc));
        /* Connection is failed so re-set one-shot timer to retry again  */
        if (channel->connectTimer)
        {
            ////////// [RE:fixme] Is it safe to delete ourselves blocking-ly and while there's still code to run?
            OSAL_DestroyTimer(channel->connectTimer);
            channel->connectTimer = NULL;
        }
        channel->connectTimer = OSAL_NewTimer(_AWS_IOT_ConnectTimerCB, _connRetryTimeout, false, channel);
        if (_connRetryTimeout < AWS_CONNECTION_MAX_RECONNECT_WAIT_INTERVAL_MS)
        {
            /* We are using exponential back-off so need to multiple next wait time by two. */
            _connRetryTimeout *= 2;
        }
        if (NULL == channel->connectTimer)
        {
            LOG_Error("Could not create AWS Connect timer.");
        }
    }
    else
    {
        LOG_Info("We have a connection on channel %i", channel->id);

        static bool gwStarted = false;
        if (!gwStarted)
        {
            EnsoErrorCode_e retVal = SYS_Start();
            if (eecNoError != retVal)
            {
                LOG_Error("SYS_Start error %s", LSD_EnsoErrorCode_eToString(retVal));
                exit(retVal);
            }

            gwStarted = true;
        }
#if 0
       // initializeRecords(&channel->mqttClient);
#endif
      //  LOG_Info("Shadow connected to %s", _connectParams.pClientID);
        /* We connected but Illuminate Online LED if only device is registered. */
        _AWS_SendLEDStatus(PROP_ONLINE_ID, _bGateWayRegistered);
        /* Connection success so we mark channel as connected to include into MQTT yield. */
        channel->bConnected = true;
        /* Mark channel as enabled. */
        channel->bEnabled = true;
        if (channel->id == 0)
        {
            /*
             * All updates are sent over Channel 0 so just inform Fault Buffers
             * about new state for channel 0
             * */
            AWS_ConnectionStateCB(true);

            // Do any pending device deletes now
            _AWS_KickDeleteWorker(NULL);

            // Toggle the online property for gateway connection (which is in
            // connection 0) so we can be certain an update is sent.
            EnsoPropertyValue_u onlnVal = { 0 };
            onlnVal.uint32Value = 0;
            EnsoErrorCode_e retVal =  LSD_SetPropertyValueByAgentSideId(
                                        COMMS_HANDLER,
                                        &_gatewayId,
                                        REPORTED_GROUP,
                                        PROP_ONLINE_ID,
                                        onlnVal);
            if (eecNoError != retVal)
            {
                LOG_Error("Failed to reset Gateway Online Property %s", LSD_EnsoErrorCode_eToString(retVal));
            }

            onlnVal.uint32Value = 1;
            retVal =  LSD_SetPropertyValueByAgentSideId(
                                        COMMS_HANDLER,
                                        &_gatewayId,
                                        REPORTED_GROUP,
                                        PROP_ONLINE_ID,
                                        onlnVal);

            if (eecNoError != retVal)
            {
                LOG_Error("Failed to set Gateway Online Property %s", LSD_EnsoErrorCode_eToString(retVal));
            }
            /*
             * For all delta updates, we use connection 0.
             * Connection 0 is re-established so send all out of sync local shadows.
             */
            if (_bGatewayAcceptRejectTopicsSubscribed)
            {
                AWS_StartOutOfSyncTimer();
            }
        }
        else
        {
            // A new connection on another channel has been observed to impact message sending
            // So reset for safety, in the worst case this means we re-send a message that was
            // successful
            AWS_SendFailedForFaultBuffer();
        }
    }
}
#endif
/**
 * \name _AWS_IOT_ConnectTimerCB
 *
 * \brief Callback function to handle periodic connection attempt in case of connection fail.
 *
 * \param  handle     Reference to client instance
 *
 * \return none
 */
static void _AWS_IOT_ConnectTimerCB(void* handle)
{
    AWS_CLD_CommsChannel_t *channel = (AWS_CLD_CommsChannel_t *)OSAL_GetTimerParam(handle);

    if (OSAL_SendMessage(_connectTimeoutMessageQueue, &channel->id, sizeof(channel->id), MessagePriority_high) < 0)
    {
        LOG_Error("OSAL_SendMessage failed %s", strerror(errno));
    }
}


/**
 * \name _AWS_IOT_ProcessConnectTimeouts
 *
 * \brief Task to handle periodic connection attempt in case of connection fail.
 *
 * \param  handle     Reference to client instance
 *
 * \return none
 */
static void _AWS_IOT_ProcessConnectTimeouts(MessageQueue_t mq)
{
    // For ever loop
    for ( ; ; )
    {
        SeqQueueItem item;
        int32_t channelId;
        MessagePriority_e priority;

        int size = OSAL_ReceiveMessage(mq, &channelId, sizeof(channelId), &priority);
        if (size != sizeof(channelId))
        {
            LOG_Error("OSAL_ReceiveMessage error %d", size);
            continue;
        }

        item.type = SeqItem_Connect;
        item.args.Connect.channelID = channelId;

        bool bQueued = _AWS_QueueSeqItem(item, SeqItemPri_High);

        if (bQueued == false)
        {
            LOG_Error("Failed to queue Connect request for channel %d!", channelId);
        }
    }
}

/******************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * \name AWS_Initialise
 *
 * \brief Initialise the AWS Comms Handler
 *
 * \param rootCAFilename        The name and path of the root certificate authority file
 *
 * \param certificateFilename   The name and path of the device certificate file
 *
 * \param privateKeyFilename    The name and path of the device key file
 *
 * \return       EnsoErrorCode_e:      0 = success else error code
 */
EnsoErrorCode_e AWS_Initialise(char * rootCA, char * certs, char * key)
{
    OSAL_InitMutex(&_commsMutex, NULL);
    OSAL_InitMutex(&_timerMutex, NULL);
    OSAL_InitMutex(&_deleteMutex, NULL);
    OSAL_InitMutex(&_seqMutex, NULL);
    EnsoErrorCode_e retVal;
    min_ms_between_deltas = atoi(getenvDefault("MIN_MS_BETWEEN_DELTAS", "25"));
    aws_polling_interval_in_ms = atoi(getenvDefault("AWS_POLLING_INTERVAL_IN_MS", "400"));
    aws_polling_yield_timeout_in_ms = atoi(getenvDefault("AWS_POLLING_YIELD_TIMEOUT_IN_MS", "100"));
    aws_out_of_sync_initial_shadow_recovery_interval_ms = atoi(getenvDefault("AWS_OUT_OF_SYNC_INITIAL_SHADOW_RECOVERY_INTERVAL_MS", "30000"));
    aws_out_of_sync_short_shadow_recovery_interval_ms = atoi(getenvDefault("AWS_OUT_OF_SYNC_SHORT_SHADOW_RECOVERY_INTERVAL_MS", "2500"));
    _commsDelayMs = aws_out_of_sync_initial_shadow_recovery_interval_ms;
    //LOG_Info("AWS IoT SDK Version %d.%d.%d%s%s, %d connections, rootCA=%s, certs=%s, key=%s",
      //  VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, *VERSION_TAG ? "-" : "", VERSION_TAG,
        //MAX_CONNECTION_PER_GATEWAY, rootCA, certs, key);
    /* Initialise all channels */
    for (int i = 0; i < MAX_CONNECTION_PER_GATEWAY; i++)
    {
        AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[i];
        CLD_CommsChannel_t* channelBase = &channel->commsChannelBase;
        channel->id = i;
        channel->bConnected = false;
        channel->bEnabled = false;
        channel->subscriptionCount = 0;
        channel->disconnectRetryCount = 0;
      //  memset(channel->awsShadowUpdateDeltaTopic, 0, sizeof(channel->awsShadowUpdateDeltaTopic));
        memset(channel->awsShadowUpdateDeltaTopicUnsubAttempts, 0,
               sizeof(channel->awsShadowUpdateDeltaTopicUnsubAttempts));

        // Initialise the comm channel
        retVal = CLD_BaseCommsChannelInitialise(channelBase, rootCA, certs, key);
        if (eecNoError != retVal)
        {
            LOG_Error("Channel base initialisation fail %d", retVal);
            return retVal;
        }
        channelBase->Open = AWS_CommsChannelOpen;
        channelBase->DeltaStart = AWS_CommsChannelDeltaStart;
        channelBase->DeltaAddPropertyString = AWS_CommsChannelDeltaAddPropertyStr;
        channelBase->DeltaAddProperty = AWS_CommsChannelDeltaAddProperty;
        channelBase->DeltaSend = AWS_CommsChannelDeltaSend;
        channelBase->DeltaFinish = AWS_CommsChannelDeltaFinish;
        channelBase->Poll = AWS_CommsChannelPoll;
        channelBase->Close = AWS_CommsChannelClose;
        channelBase->Destroy = AWS_CommsChannelDestroy;
    }
    /* Set common connection parameters */
    //_connectParams = iotClientConnectParamsDefault;
    //_connectParams.keepAliveIntervalInSec = AWS_PING_INTERVAL_IN_SEC;
    //_connectParams.MQTTVersion = MQTT_3_1_1;
    //_connectParams.isCleanSession = true;
    //_connectParams.isWillMsgPresent = false;
    //_connectParams.pPassword = NULL;
    //_connectParams.pUsername = NULL;
    //_connectParams.will.qos = QOS0;
    if (retVal == eecNoError)
    {
        retVal = AWS_FaultBufferInit();
    }
    if (retVal == eecNoError)
    {
        retVal = AWS_TimestampInit();
    }
    // Create message queue for enso communications
    const char * name = "/AWSC";
    _messageQueue = OSAL_NewMessageQueue(name,
                        ECOM_MAX_MESSAGE_QUEUE_DEPTH,
                        ECOM_MAX_MESSAGE_SIZE);
    ECOM_RegisterOnUpdateFunction(COMMS_HANDLER, AWS_FaultBuffer);
    ECOM_RegisterMessageQueue(COMMS_HANDLER, _messageQueue);
    // Create listening thread
    listeningThread = OSAL_NewThread(_AWS_MessageQueueListener, _messageQueue);
    if (listeningThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for AWS Comms Handler");
        return eecInternalError;
    }
    // Create Sequencer thread
    sequencerThread = OSAL_NewThread(_AWS_Sequencer, NULL);
    if (sequencerThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for AWS Comms Sequencer Handler");
        return eecInternalError;
    }

    // Set led to initial state.
    _AWS_SendLEDStatus(PROP_ONLINE_ID, false);

    // Create object delete thread in order to make deleting robust
    _deleteQueue = NULL;
    name = "/AWSD";
    _deleteMessageQueue = OSAL_NewMessageQueue(name,
                        ECOM_MAX_MESSAGE_QUEUE_DEPTH,
                        ECOM_MAX_MESSAGE_SIZE);
    deletingThread = 1;/*OSAL_NewThread(_AWS_ObjectDeleteWorker, _deleteMessageQueue);*/
    if (deletingThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for AWS Comms Handler delete thread");
        return eecInternalError;
    }

    // Create connect message queue to handle connect timer callbacks
    name = "/AWST";
    _connectTimeoutMessageQueue = OSAL_NewMessageQueue(name,
                                ECOM_MAX_MESSAGE_QUEUE_DEPTH,
                                sizeof(int32_t));
    connectTimeoutThread = OSAL_NewThread(_AWS_IOT_ProcessConnectTimeouts, _connectTimeoutMessageQueue);
    if (connectTimeoutThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for AWS Comms Handler connect timeout thread");
        return eecInternalError;
    }

    if (OSAL_InitBinarySemaphore(&_outOfSyncSemaphore) != 0)
    {
        LOG_Error("OSAL_InitBinarySemaphore failed for AWS Comms Handler out of sync timeout semaphore");
        return eecInternalError;
    }
    outOfSyncThread = OSAL_NewThread(_AWS_IOT_SendOutOfSyncShadows, NULL);
    if (outOfSyncThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for AWS Comms Handler out of sync timeout thread");
        return eecInternalError;
    }
    return retVal;
}

/**
 * \brief   This function creates the AWS connection and starts polling it.
 *
 * \param   gatewayId       The gateway identifier
 *
 * \return  EnsoErrorCode_e
 */
EnsoErrorCode_e AWS_Start(EnsoDeviceId_t* gatewayId)
{
    /* Mark AWS Module as started */
    _bAWSStarted = true;

    SYS_GetDeviceId(&_gatewayId);

    /* To start AWS module, just start the channel 0 */
    AWS_CLD_CommsChannel_t* channel0 = &_awsCommsChannels[0];

    EnsoErrorCode_e retVal = channel0->commsChannelBase.Open(&channel0->commsChannelBase, gatewayId);
    if (eecNoError != retVal)
    {
        LOG_Error("baseChannel->Open failed %s", LSD_EnsoErrorCode_eToString(retVal));
    }

    /* Enable Polling */
    //////// [RE:workaround] Don't use a timer, use a thread instead
    /*
    _AwsPollingTimer = OSAL_NewTimer(_AWS_PollingPass, aws_polling_interval_in_ms, true, NULL);
    if (NULL == _AwsPollingTimer)
    {
        LOG_Error("Could not create AWS polling timer.");
        retVal = eecInternalError;
    }
    */
#if 0
    _AwsPollingThread = OSAL_NewThread(_AWS_PollingPass, _deleteMessageQueue);
#endif
    if (_AwsPollingThread == 0)
    {
        return eecInternalError;
    }

    return retVal;
}

/**
 * Stops the AWS Module and all its internal services.
 *
 * \param none
 *
 * \return none
 *
 */
void AWS_Stop(void)
{
    /* Mark AWS module as stopped */
    _bAWSStarted = false;

    /* Terminate internal timers */
    if (NULL != _AwsPollingTimer)
    {
        OSAL_DestroyTimer(_AwsPollingTimer);
    }

    /* Stop AWS Fault Buffers */
    AWS_FaultBufferStop();

    if (NULL != outOfSyncShadowTimer)
    {
        OSAL_DestroyTimer(outOfSyncShadowTimer);
    }

    for (int i = 0; i < MAX_CONNECTION_PER_GATEWAY; i++)
    {
        if (NULL != _awsCommsChannels[i].connectTimer)
        {
            OSAL_DestroyTimer(_awsCommsChannels[i].connectTimer);
        }
    }

    /* Close all connections */
    for (int i = 0; i < MAX_CONNECTION_PER_GATEWAY; i++)
    {
        AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[i];

        if (channel->bConnected)
        {
           AWS_CommsChannelClose((CLD_CommsChannel_t*)channel);
        }
    }

    /* Kill Internal Threads */
    OSAL_KillThread(sequencerThread);
    OSAL_KillThread(listeningThread);
    OSAL_KillThread(deletingThread);
}

/**
 * \name AWS_IOT_ShadowConnect
 *
 * \brief This function sets up the MQTT connection to the Platform.
 *
 * \param   pClient             Pointer to the MQTT client
 *
 * \param   gatewayId           The gateway identifier used for the Shadow thing Id
 *
 * \return                      Error code.
 */
EnsoErrorCode_e AWS_IOT_ShadowConnect(
        AWS_CLD_CommsChannel_t *channel,
        EnsoDeviceId_t *gatewayId)
{
    // Sanity check
    if (!channel || !gatewayId)
    {
        return eecNullPointerSupplied;
    }

    // All MqTT messages published by a real device must set the client ID
    // to be the gateway’s ID (which will be used as its thing ID once it
    // has registered). (See G2-410)
    // The environment variable AWS_IOT_MQTT_CLIENT_ID can be used in
    // testing to provide a MQTT client Id but isn't required for the real
    // platform.
    ////////snprintf(channel->clientName, MQTT_CLIENT_NAME_BUFFER_SIZE, "%s_%02d", _gatewayThingName, channel->id);	//////// [RE:cast]
    snprintf(channel->clientName, MQTT_CLIENT_NAME_BUFFER_SIZE, "%s_%02d", _gatewayThingName, (int)(channel->id));
    //_connectParams.pClientID = channel->clientName;
    //_connectParams.clientIDLen = (uint16_t) strlen(_connectParams.pClientID);

    EnsoErrorCode_e retVal = eecNoError;

    if (channel->id == 0)
    {
        retVal = _AWS_GenerateLastWillAndTestament();

        /* Will message is just for channel 0 */
      //  _connectParams.isWillMsgPresent = true;
    }
    else
    {
       // _connectParams.isWillMsgPresent = false;
    }

   // _connectParams.will.isRetained = false;

    /* Set timer to Connect to Cloud */
    channel->connectTimer = OSAL_NewTimer(_AWS_IOT_ConnectTimerCB, AWS_CONNECTION_INITIAL_CONNECT_TIMEOUT, false, channel);

    if (NULL == channel->connectTimer)
    {
        LOG_Error("Could not create AWS Connect timer.");
    }

    return retVal;
}


/**
 * \name AWS_CommsChannelOpen
 *
 * \brief Defines function pointer for opening the communications channel.
 *
 * This function is responsible for acquiring (as opposed to allocating) the
 * memory and other resources needed to maintain a communications interface
 * between the ensoAgent and the ensoCloud.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   gatewayId           The gateway identifier used for the Shadow thing Id
 *
 * \return                      Error code.
 */
EnsoErrorCode_e AWS_CommsChannelOpen(CLD_CommsChannel_t* commsChannel, EnsoDeviceId_t* gatewayId)
{
    /* Sanity check */
    if (!commsChannel || !gatewayId)
    {
        return eecNullPointerSupplied;
    }

    /* Initialise the MQTT client */
    char port [16];
    //snprintf(port, sizeof port, "%d", AWS_IOT_MQTT_PORT);
    AWS_CLD_CommsChannel_t* channel = (AWS_CLD_CommsChannel_t* ) commsChannel;
   // char * aws_iot_mqtt_host = getenvDefault("AWS_IOT_MQTT_HOST", AWS_IOT_MQTT_HOST);
    //char * aws_iot_mqtt_port = getenvDefault("AWS_IOT_MQTT_PORT", port);
    //LOG_Info("%s:%s", aws_iot_mqtt_host, aws_iot_mqtt_port);
#if 0
    channel->shadowParameters.pHost = aws_iot_mqtt_host;
    //channel->shadowParameters.port  = atoi(aws_iot_mqtt_port);
    channel->shadowParameters.pClientCRT = commsChannel->clientCertificateFilename;
    channel->shadowParameters.pClientKey = commsChannel->clientKeyFilename;
    channel->shadowParameters.pRootCA = commsChannel->rootCertifcateAuthorityFilename;
    channel->shadowParameters.enableAutoReconnect = false;
    channel->shadowParameters.disconnectHandler = _AWS_DisconnectCallbackHandler;
    LOG_Info("%s:%d", channel->shadowParameters.pHost, channel->shadowParameters.port);
#endif
    EnsoErrorCode_e retVal = eecNoError;

    LOG_Trace("Calling aws_iot_shadow_init");
    //IoT_Error_t rc = aws_iot_shadow_init(&(channel->mqttClient), &(channel->shadowParameters));
    if (1)
    {
      //  LOG_Error("aws_iot_shadow_init failed %s", _AWS_Strerror(rc));
        retVal = eecLocalShadowConnectionFailedToInitialise;
    }

    if (eecNoError == retVal)
    {
        // Set the gateway name
        int bufferUsed;
        EnsoErrorCode_e retVal = LSD_GetThingName(_gatewayThingName, sizeof(_gatewayThingName), &bufferUsed, *gatewayId, _gatewayId);
        assert(eecNoError == retVal);

       // rc = AWS_IOT_ShadowConnect(channel, gatewayId);

        if (1)
        {
        //    LOG_Error("aws_iot_shadow_connect failed %s", _AWS_Strerror(rc));
            retVal = eecLocalShadowConnectionFailedToConnect;
        }
    }

    return retVal;
}

/**
 * \name AWS_CommsChannelDeltaStart
 *
 * \brief Defines function pointer for initialising a delta message
 *
 * This function begins the process of creating a delta message ready to be
 * filled using Add before being Sent or Finished.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   propertyGroup       The group of the properties in the delta message
 *
 * \return  Error code
 */
EnsoErrorCode_e AWS_CommsChannelDeltaStart(CLD_CommsChannel_t* commsChannel, const PropertyGroup_e propertyGroup)
{
    /* Sanity check */
    if (!commsChannel)
    {
        return eecNullPointerSupplied;
    }

    AWS_CLD_CommsChannel_t* channel = (AWS_CLD_CommsChannel_t*) commsChannel;
    channel->numJsonEntriesInBuffer = 0;

    EnsoErrorCode_e retVal = eecNoError;
    //IoT_Error_t rc = aws_iot_shadow_init_json_document(channel->jsonDocumentBuffer, AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER);
    if ( 1)
    {
        retVal = eecLocalShadowDocumentFailedToInitialise;
    }

    int bufferOffset = strlen(channel->jsonDocumentBuffer);
    int bufferRemaining = AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER - bufferOffset;

    if (eecNoError == retVal && bufferRemaining > 1)
    {
        int snprintfReturn = -1;
        switch (propertyGroup)
        {
            case DESIRED_GROUP:
                snprintfReturn = snprintf(channel->jsonDocumentBuffer + bufferOffset, bufferRemaining, "\"desired\":{");
                break;

            case REPORTED_GROUP:
                snprintfReturn = snprintf(channel->jsonDocumentBuffer + bufferOffset, bufferRemaining, "\"reported\":{");
                break;
            default:
                assert(0);
                break;
        }

        if (snprintfReturn <= 0)
        {
            retVal = eecConversionFailed;
            LOG_Error("snprintf returned %d", snprintfReturn)
        }
    }

    return retVal;
}


/**
 * \name AWS_CommsChannelDeltaAddPropertyStr
 *
 * \brief Defines function pointer for adding a string property
 *
 *  This function adds a string to the delta message that has already been
 *  started.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   propertyString          The property including ID in string form
 *
 * \return                          Error code.
 */
EnsoErrorCode_e AWS_CommsChannelDeltaAddPropertyStr  ( CLD_CommsChannel_t* commsChannel, const char* propertyString )
{
    /* Sanity check */
    if (!commsChannel) return eecNullPointerSupplied;
    if (!propertyString) return eecNullPointerSupplied;

    return eecNoError;
}

/**
 * \name AWS_CommsChannelDeltaAddProperty
 *
 * \brief Add a property to the delta
 *
 * This function adds a property to the delta.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   property            The property object
 *
 * \param   firstNestedProperty Is it our first nested property
 *
 * \param   group               The property group (desired or reported)
 *
 * \return  Error code
 */
EnsoErrorCode_e AWS_CommsChannelDeltaAddProperty(CLD_CommsChannel_t* commsChannel,
        const EnsoProperty_t* property, bool firstNestedProperty, PropertyGroup_e group)
{
    AWS_CLD_CommsChannel_t* channel =(AWS_CLD_CommsChannel_t* ) commsChannel;

    /* Sanity check */
    if (!channel || !property)
    {
        return eecNullPointerSupplied;
    }

    int bufferOffset = strlen(channel->jsonDocumentBuffer);
    int bufferRemaining = AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER - bufferOffset;

    /* Check if property is a string (memory blob) which is too large to be converted
     * to a JSON due to the limitation of AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER (400 bytes)
     */
    if ((REPORTED_GROUP == group) &&
        (evBlobHandle == property->type.valueType) &&
        property->reportedValue.memoryHandle &&
        (strlen((char*)property->reportedValue.memoryHandle) > bufferRemaining)
       )
    {
        LOG_Warning("Property %s too large to be converted to JSON for AWS delta buffer", property->cloudName);
        return eecBufferTooSmall;
    }

    EnsoErrorCode_e retVal = eecNoError;
    if (bufferRemaining > 1)
    {
        // Check if this is the first property in the delta
        if (channel->numJsonEntriesInBuffer)
        {
            channel->jsonDocumentBuffer[bufferOffset++] = ',';
            channel->jsonDocumentBuffer[bufferOffset] = '\0';
            bufferRemaining--;
        }

        // Write the key part of the key-value pair
        char parentName[LSD_PROPERTY_NAME_BUFFER_SIZE];
        char childName[LSD_PROPERTY_NAME_BUFFER_SIZE];
        bool nestedProperty = LSD_IsPropertyNested(property->cloudName, parentName, childName);

        int snprintfReturn;
        if (nestedProperty && firstNestedProperty)
        {
            // Only for the first nested property do we create group name using parent
            snprintfReturn = snprintf(channel->jsonDocumentBuffer + bufferOffset,
                    bufferRemaining, "\"%s\":{\"%s\":", parentName, childName);
        }
        else if (nestedProperty && !firstNestedProperty)
        {
            // We know we have already added the first nested property in group,
            // so we are safe to remove previously closing characters which are '},'
            bufferOffset -= 2;
            bufferRemaining += 2;
            snprintfReturn = snprintf(channel->jsonDocumentBuffer + bufferOffset,
                    bufferRemaining, ",\"%s\":", childName);
        }
        else
        {
            snprintfReturn = snprintf(channel->jsonDocumentBuffer + bufferOffset,
                    bufferRemaining, "\"%s\":", property->cloudName);
        }

        if (snprintfReturn < 0)
        {
            retVal = eecConversionFailed;
            LOG_Error("snprintf returned %d", snprintfReturn);
        }
        else if (snprintfReturn > bufferRemaining)
        {
            retVal = eecBufferTooSmall;
        }
        else
        {
            // Write the value part of the key-value pair
            if (retVal == eecNoError)
            {
                int bytesUsed = 0;
                bufferOffset += snprintfReturn;
                bufferRemaining = AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER - bufferOffset;
                bufferOffset = strlen(channel->jsonDocumentBuffer);

                channel->numJsonEntriesInBuffer++;
                retVal = LSD_ConvertTypedDataToJsonValue(
                        channel->jsonDocumentBuffer + bufferOffset,
                        bufferRemaining,
                        &bytesUsed,
                        (group == REPORTED_GROUP) ? property->reportedValue : property->desiredValue,
                        property->type.valueType);

                bufferOffset += snprintfReturn;
                bufferRemaining = AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER - bufferOffset;
                bufferOffset = strlen(channel->jsonDocumentBuffer);

                if (nestedProperty)
                {
                    snprintf(channel->jsonDocumentBuffer + bufferOffset, bufferRemaining, "}");
                }
            }
            else
            {
                retVal = eecConversionFailed;
            }
        }
    }
    else
    {
        retVal = eecBufferTooSmall;
    }

    return retVal;
}


/**
 * \name AWS_CommsChannelDeltaSend
 *
 * \brief Defines function pointer for sending the delta message
 *
 * This function sends the delta message .
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   deviceId            The device that is being updated by the delta
 *
 * \param   context             A value that we can use in the callback to determine
 *                                   which delta the callback is about
 *
 * \return                      Error code.
 */
EnsoErrorCode_e AWS_CommsChannelDeltaSend(CLD_CommsChannel_t* commsChannel, const EnsoDeviceId_t deviceId, void* context)
{
    /* Sanity check */
    if (!commsChannel)
    {
        return eecNullPointerSupplied;
    }
    static uint32_t lasttime = 0;
    uint32_t now = OSAL_time_ms();
    uint32_t delta = now - lasttime;
    if (delta < min_ms_between_deltas)
    {
        OSAL_sleep_ms(min_ms_between_deltas - delta);
        now = OSAL_time_ms();
    }
    lasttime = now;
    AWS_CLD_CommsChannel_t* channel = (AWS_CLD_CommsChannel_t* ) commsChannel;
    char * buffer = channel->jsonDocumentBuffer;
    int bufferOffset = strlen(buffer);
    int bufferRemaining = AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER - bufferOffset;
    /* Need to close the delta with "}," */
    if (bufferRemaining < 3)
    {
       return eecBufferTooSmall;
    }
    snprintf(buffer + bufferOffset, bufferRemaining, "}," );
    EnsoErrorCode_e retVal = eecNoError;
    //IoT_Error_t rc;
  //  rc = aws_iot_finalize_json_document(buffer, AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER);
    if (1 )
    {
       // LOG_Error("aws_iot_shadow_finalize_json_document %s", _AWS_Strerror(rc));
        retVal = eecLocalShadowDocumentFailedToInitialise;
    }
    else
    {
        char name[ENSO_OBJECT_NAME_BUFFER_SIZE];
        int bufferUsed;
        retVal = LSD_GetThingName(name, sizeof name, &bufferUsed, deviceId, _gatewayId);
        assert(eecNoError == retVal);
        LOG_Info("--> %d:%s", strlen(buffer), buffer);
        OSAL_LockMutex(&_commsMutex);
        _bUpdateInProgress = true;
       // rc = aws_iot_shadow_update(&channel->mqttClient, name, buffer, _AWS_CommsChannelCallback, context, 1, false);
        _bUpdateInProgress = false;
        OSAL_UnLockMutex(&_commsMutex);
        if (1)
        {
            AWS_SendFailedForFaultBuffer(); // Won't get the callback
            retVal = eecLocalShadowDocumentFailedToSendDelta;
       //     LOG_Error("aws_iot_shadow_update %s", _AWS_Strerror(rc));
        }
        else
        {
            //LOG_Info("aws_iot_shadow_update: %s", buffer);
        }
    }

    return retVal;
}


/**
 * \name AWS_CommsChannelDeltaFinish
 *
 * \brief Defines function pointer for clearing down the delta message
 *
 * This function clears the buffers used by the delta message ready for a new
 * message to be started.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return                          Error code.
 */
EnsoErrorCode_e AWS_CommsChannelDeltaFinish(CLD_CommsChannel_t* commsChannel)
{
    EnsoErrorCode_e retVal = eecNoError;
    AWS_CLD_CommsChannel_t* channel =(AWS_CLD_CommsChannel_t* ) commsChannel;

    /* Sanity check */
    if (!channel) return eecNullPointerSupplied;

    /* Actually there isn't really anything we do here at the moment. */
    return retVal;
}


/**
 * \name AWS_CommsChannelPoll
 *
 * \brief Performs a single pass of the polling loop.
 *
 * This function polls the communication channel.
 *
 * \note All callbacks ever used in the AWS SDK will be executed in the context of this function.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return                      Error code
 */
EnsoErrorCode_e AWS_CommsChannelPoll(CLD_CommsChannel_t* commsChannel)
{
    /* Sanity check */
    if (!commsChannel) return eecNullPointerSupplied;

    AWS_CLD_CommsChannel_t* channel = (AWS_CLD_CommsChannel_t*) commsChannel;

    OSAL_LockMutex(&_commsMutex);
    //IoT_Error_t rc = aws_iot_shadow_yield(&channel->mqttClient, aws_polling_yield_timeout_in_ms);
    OSAL_UnLockMutex(&_commsMutex);

    EnsoErrorCode_e retVal = eecNoError;
    switch (true)
    {
      //  case NETWORK_ATTEMPTING_RECONNECT:
       // case NETWORK_RECONNECTED:
         //   yieldFailureCount[channel->id] = 0;
          //  break;

        //case SUCCESS:
          //  yieldFailureCount[channel->id] = 0;
            //break;

        default:
         //   LOG_Error("aws_iot_shadow_yield %s", _AWS_Strerror(rc));
            retVal = eecLocalShadowConnectionPollingFailed;
            // Has the failed count been exceeded for this channel?
            yieldFailureCount[channel->id]++;
            if (yieldFailureCount[channel->id] > AWS_YIELD_RETRY_COUNT)
            {
                LOG_Info("Yield failure limit reached");
                SeqQueueItem item;
                item.type = SeqItem_Disconnect;
                item.args.Connect.channelID = channel->id;
                bool bPush = _AWS_QueueSeqItem(item, SeqItemPri_High);

                if (!bPush)
                {
                    LOG_Warning("Failed to add Sequencer Queue!");
                }

                item.type = SeqItem_Connect;
                item.args.Connect.channelID = channel->id;
                bPush = _AWS_QueueSeqItem(item, SeqItemPri_High);

                if (!bPush)
                {
                    LOG_Warning("Failed to add Sequencer Queue!");
                }
            }
            break;
    }

    return retVal;
}


/**
 * \name AWS_CommsChannelClose
 *
 * \brief Defines function pointer to close down the communications channel
 *
 * This function closes the communication channel.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return                          Error code.
 */
EnsoErrorCode_e AWS_CommsChannelClose(CLD_CommsChannel_t* commsChannel)
{
    AWS_CLD_CommsChannel_t* channel = (AWS_CLD_CommsChannel_t* ) commsChannel;

    /* Sanity check */
    if (!channel)
    {
        return eecNullPointerSupplied;
    }

    EnsoErrorCode_e retVal = eecNoError;
    OSAL_LockMutex(&_commsMutex);
    //IoT_Error_t rc = aws_iot_shadow_disconnect(&channel->mqttClient);
    OSAL_UnLockMutex(&_commsMutex);

    if (1)
    {
        //LOG_Error("aws_iot_shadow_disconnect %s", _AWS_Strerror(rc));
        retVal = eecLocalShadowConnectionFailedToClose;
    }

    channel->bConnected = false;

    return retVal;
}


/**
 * \name AWS_CommsChannelDestroy
 *
 * \brief Defines function pointer for destroying the communications channel.
 * Free any resources used by the communications channel and make sure
 * everything is cleaned up.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return                          Error code.
 */
EnsoErrorCode_e AWS_CommsChannelDestroy(CLD_CommsChannel_t* commsChannel)
{
    EnsoErrorCode_e retVal = eecNoError;

    /* Sanity check */
    if (!commsChannel) return eecNullPointerSupplied;

    return retVal;
}

/**
 * \name AWS_RegisterNewDeltaRequest
 *
 * \brief Once a new delta is ready to send, this function can be used to ping
 * AWS module about new delta.
 *
 * \param none
 *
 * \return true if request is accepted otherwise false
 */
bool AWS_RegisterNewDeltaRequest(void)
{
    SeqQueueItem item;
    item.type = SeqItem_Delta;

    /* Just queue new delta request to AWS Sequencer */
    return _AWS_QueueSeqItem(item, SeqItemPri_Low);
}

/**
 * \name _AWS_Strerror
 *
 * \brief returns string representation of aws error code
 *
 * \param errorCode
 *
 * \return string
 *
 */
#if 0
static char * _AWS_Strerror(IoT_Error_t error)
{
    return error == NETWORK_PHYSICAL_LAYER_CONNECTED ? "physical layer connected" :
           error == NETWORK_MANUALLY_DISCONNECTED ? "manually disconnected" :
           error == NETWORK_ATTEMPTING_RECONNECT ? "reconnect attempt in progress" :
           error == NETWORK_RECONNECTED ? "reconnected" :
           error == MQTT_NOTHING_TO_READ ? "MQTT buffer empty" :
           error == MQTT_CONNACK_CONNECTION_ACCEPTED ? "MQTT connection accepted" :
           error == SUCCESS ? "Success" :
           error == FAILURE ? "Failure" :
           error == NULL_VALUE_ERROR ? "Null value error" :
           error == TCP_CONNECTION_ERROR ? "TCP connection error" :
           error == SSL_CONNECTION_ERROR ? "SSL handshake error" :
           error == TCP_SETUP_ERROR ? "TCP setup error" :
           error == NETWORK_SSL_CONNECT_TIMEOUT_ERROR ? "SSL connect timeout" :
           error == NETWORK_SSL_WRITE_ERROR ? "SSL write error" :
           error == NETWORK_SSL_INIT_ERROR ? "SSL initialization error" :
           error == NETWORK_SSL_CERT_ERROR ? "SSL Certificate Error" :
           error == NETWORK_SSL_WRITE_TIMEOUT_ERROR ? "SSL Write timed out" :
           error == NETWORK_SSL_READ_TIMEOUT_ERROR ? "SSL Read timed out" :
           error == NETWORK_SSL_READ_ERROR ? "SSL Read Error" :
           error == NETWORK_DISCONNECTED_ERROR ? "disconnected" :
           error == NETWORK_RECONNECT_TIMED_OUT_ERROR ? "reconnect timed out" :
           error == NETWORK_ALREADY_CONNECTED_ERROR ? "already connected" :
           error == NETWORK_MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED ? "Random number generator seeding" :
           error == NETWORK_SSL_UNKNOWN_ERROR ? "SSL Unknown Error" :
           error == NETWORK_PHYSICAL_LAYER_DISCONNECTED ? "physical layer disconnected" :
           error == NETWORK_X509_ROOT_CRT_PARSE_ERROR ? "root certificate invalid" :
           error == NETWORK_X509_DEVICE_CRT_PARSE_ERROR ? "device certificate invalid" :
           error == NETWORK_PK_PRIVATE_KEY_PARSE_ERROR ? "private key parse" :
           error == NETWORK_ERR_NET_SOCKET_FAILED ? "socket failed" :
           error == NETWORK_ERR_NET_UNKNOWN_HOST ? "unknown host" :
           error == NETWORK_ERR_NET_CONNECT_FAILED ? "connect failed" :
           error == NETWORK_SSL_NOTHING_TO_READ ? "nothing to read" :
           error == MQTT_CONNECTION_ERROR ? "MQTT connection error" :
           error == MQTT_CONNECT_TIMEOUT_ERROR ? "MQTT handshake timeout" :
           error == MQTT_REQUEST_TIMEOUT_ERROR ? "MQTT request timeout" :
           error == MQTT_UNEXPECTED_CLIENT_STATE_ERROR ? "MQTT client state error" :
           error == MQTT_CLIENT_NOT_IDLE_ERROR ? "MQTT client not idle" :
           error == MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR ? "MQTT RX invalid packet type" :
           error == MQTT_RX_BUFFER_TOO_SHORT_ERROR ? "MQTT RX buffer too short" :
           error == MQTT_TX_BUFFER_TOO_SHORT_ERROR ? "MQTT TX buffer too short" :
           error == MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR ? "MQTT max subscritions reached" :
           error == MQTT_DECODE_REMAINING_LENGTH_ERROR ? "MQTT decode remaining length" :
           error == MQTT_CONNACK_UNKNOWN_ERROR ? "MQTT Connect request unknown" :
           error == MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR ? "MQTT Connect request unacceptable protocol version" :
           error == MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR ? "MQTT Connect request identifier rejected" :
           error == MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR ? "MQTT Connect request unavailable" :
           error == MQTT_CONNACK_BAD_USERDATA_ERROR ? "MQTT Connect request bad userdata" :
           error == MQTT_CONNACK_NOT_AUTHORIZED_ERROR ? "MQTT Connect request not authorized" :
           error == JSON_PARSE_ERROR ? "JSON parse error" :
           error == SHADOW_WAIT_FOR_PUBLISH ? "Shadow wait for publish" :
           error == SHADOW_JSON_BUFFER_TRUNCATED ? "Shadow JSON buffer truncated" :
           error == SHADOW_JSON_ERROR ? "Shadow JSON error" :
           error == MUTEX_INIT_ERROR ? "Mutex init" :
           error == MUTEX_LOCK_ERROR ? "Mutex lock" :
           error == MUTEX_UNLOCK_ERROR ? "Mutex unlock" :
           error == MUTEX_DESTROY_ERROR ? "Mutex destroy" : "Unknown error";
   }
#endif

/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * \brief  This function polls the comms channel
 *
 * Called on timer expiry "as if" it were the start function of a new thread.
 *
 * \param handle    The comms channel handle
 */
static void _AWS_PollingPass(void * handle)
{
    //////// [RE:workaround]
    for (;;)
    {
        /* Active Channel ID */
        static int32_t activeChannelID = 0;

        AWS_CLD_CommsChannel_t* awsChannel;

        for (int i = 0; i < MAX_CONNECTION_PER_GATEWAY; i++)
        {
            awsChannel = &_awsCommsChannels[activeChannelID];

            /* Just yield to enabled channels */
            if (awsChannel->bConnected)
            {
                SeqQueueItem item;
                item.type = SeqItem_Poll;
                item.args.Poll.channelID = activeChannelID;

                bool bPush = _AWS_QueueSeqItem(item, SeqItemPri_Low);

                if (!bPush)
                {
                    LOG_Warning("Failed to add Sequencer Queue!");
                }

                activeChannelID = (activeChannelID + 1) % MAX_CONNECTION_PER_GATEWAY;

                break;
            }
            else
            {
                /* Just try next channel */
                activeChannelID = (activeChannelID + 1) % MAX_CONNECTION_PER_GATEWAY;
            }
        }


        OSAL_sleep_ms(aws_polling_interval_in_ms);
    }
}


/**
 * \brief  This function will be called from the context of aws_iot_shadow_yield() context
 *
 * \param pThingName Thing Name of the response received
 *
 * \param action The response of the action
 *
 * \param status Informs if the action was Accepted/Rejected or Timed out
 *
 * \param pReceivedJsonDocument Received JSON document
 *
 * \param pContextData the void* data passed in during the action call(update, get or delete)
 */
static void _AWS_CommsChannelCallback(
        const char *pThingName,
       // ShadowActions_t action,
        //Shadow_Ack_Status_t status,
        const char *pReceivedJsonDocument,
        void *pContextData)
{
#if 0
    if (status == SHADOW_ACK_TIMEOUT)
    {
        LOG_Error("Update Timeout thing=%s doc=%s", pThingName, pReceivedJsonDocument);
        AWS_SendFailedForFaultBuffer();

        /* Idea here is that the timeout takes around 20s, this rates our retries
         * so we don't spin here very quickly, but we do keep trying over and over
         * again until it succeeds.
         *
         * Firstly, we need to back off as network is failing
         */

        _commsDelayMs = aws_out_of_sync_initial_shadow_recovery_interval_ms;
    }
    else if (status == SHADOW_ACK_REJECTED)
    {
        LOG_Error("Update Rejected thing=%s doc=%s", pThingName, pReceivedJsonDocument);
        AWS_SendFailedForFaultBuffer();
    }
    else if (status == SHADOW_ACK_ACCEPTED)
    {
        if (pContextData)
        {
            AWS_FinishedWithFaultBuffer(pContextData);
        }

        if (action == SHADOW_UPDATE)
        {
            _AWS_ClearOutOfSyncFlag(pThingName, pReceivedJsonDocument);
        }
        else
        {
            LOG_Error("Wasn't a shadow update, what was it?")
        }
    }
#endif
}


/**
 * \brief  This function will be called from the context of aws_iot_shadow_yield() context
 *
 * \param pThingName Thing Name of the response received
 *
 * \param action The response of the action
 *
 * \param status Informs if the action was Accepted/Rejected or Timed out
 *
 * \param pReceivedJsonDocument Received JSON document
 *
 * \param pContextData the void* data passed in during the action call(update, get or delete)
 */
static void _AWS_CommsChannelDeleteCallback(
        const char *pThingName,
       // ShadowActions_t action,
        //Shadow_Ack_Status_t status,
        const char *pReceivedJsonDocument,
        void *pContextData)
{
#if 0
    if (status == SHADOW_ACK_TIMEOUT)
    {
        LOG_Error("We timed out attempting to delete a property from AWS");
        LOG_Error("Things = %s, Response = %s", pThingName, pReceivedJsonDocument);

    }
    else if (status == SHADOW_ACK_REJECTED)
    {
        LOG_Error("Property delete for AWS was rejected?");
        LOG_Error("Things = %s, Response = %s", pThingName, pReceivedJsonDocument);
    }
    else if (status == SHADOW_ACK_ACCEPTED)
    {
        // No problem
    }
#endif
}


/**
 * \brief  This function processes a delete message
 *
 * \param message The property that has been deleted
 *
 */
static void _AWS_ProcessDeletedMessage(ECOM_PropertyDeletedMessage_t* message)
{
    assert(message);
    assert(message->messageId == ECOM_PROPERTY_DELETED);
    assert(message->cloudSideId);

    // Only need cloud name to delete property, it could be nested though
    char parentName[LSD_PROPERTY_NAME_BUFFER_SIZE];
    char childName[LSD_PROPERTY_NAME_BUFFER_SIZE];
    bool nestedProperty = LSD_IsPropertyNested(message->cloudSideId, parentName, childName);

    char json[AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER];

    int snprintfReturn;

    if (nestedProperty)
    {
        snprintfReturn = snprintf(json, AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER,
                "{\"state\":{\"desired\":{\"%s\":{\"%s\":null}}, \"reported\":{\"%s\":{\"%s\":null}}},\"clientToken\":\"",
                parentName, childName, parentName, childName);
    }
    else
    {
        snprintfReturn = snprintf(json, AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER,
                "{\"state\":{\"desired\":{\"%s\":null}, \"reported\":{\"%s\":null}},\"clientToken\":\"",
                message->cloudSideId, message->cloudSideId);
    }

    if (snprintfReturn == 0)
    {
        LOG_Error("Failed to generate json delta string?");
    }
    else if (snprintfReturn == AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER)
    {
        LOG_Error("Overrun our JSON buffer when trying to generate delete message");
    }
    else
    {
        // Generate new client token - returns 0 on success
       // int clientReturn = aws_iot_fill_with_client_token(&json[snprintfReturn], AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER-snprintfReturn);

        if (1)
        {
            LOG_Error("Failed to generate client token value?");
        }
        else
        {
            //////// [RE:workaround] We don't have strnlen
            ////////int index = strnlen(json, AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER);
            int index = strlen(json);
            if (index > AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER)
            {
                index = AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER;
            }

            if (index < (AWS_MAX_LENGTH_OF_UPDATE_JSON_BUFFER-3))
            {
                json[index++] = '\"';
                json[index++] = '}';
                json[index++] = 0;

                char deviceName[ENSO_OBJECT_NAME_BUFFER_SIZE];
                int bufferUsed;
                EnsoErrorCode_e retVal = LSD_GetThingName(deviceName, sizeof(deviceName), &bufferUsed, message->deviceId, _gatewayId);
                assert(eecNoError == retVal);

                // Always use channel 0 to send updates (deltas), delete is supposed to go on update channel too
                AWS_CLD_CommsChannel_t* channel0 = &_awsCommsChannels[0];

                OSAL_LockMutex(&_commsMutex);
                _bUpdateInProgress = true;
#if 0
                IoT_Error_t rc = aws_iot_shadow_update(&(channel0->mqttClient),
                                           (char*)deviceName,
                                           json,
                                           _AWS_CommsChannelDeleteCallback,
                                           NULL,
                                           1,
                                           false);
#endif
                _bUpdateInProgress = false;
                OSAL_UnLockMutex(&_commsMutex);
                if (1)
                {
                //    LOG_Error("Failed to send delete request : %s", _AWS_Strerror(rc));
                }
            }
            else
            {
                LOG_Error("Overrun our JSON buffer when trying to terminate delete message");
            }
        }
    }
}



/**
 * \brief  This function processes the thing status
 *
 * \param deviceStatusMessage device status object
 *
 */
static void _AWS_ProcessThingStatus(ECOM_ThingStatusMessage_t* deviceStatusMessage)
{
    EnsoErrorCode_e retVal = eecNoError;

    switch (deviceStatusMessage->deviceStatus)
    {
        case THING_DISCOVERED:
        {
            // As soon as new thing is discovered, stop sending properties
            // until announce is complete. We achieve this by noting that
            // comms handler is busy, by calling this function.
            AWS_ForceBackoffDuringDiscovery();

            LOG_Info("*** ECOM_THING_STATUS message, THING_DISCOVERED");
            char parentName[ENSO_OBJECT_NAME_BUFFER_SIZE];
            char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];
            int bufferUsed;

            EnsoObject_t* owner = LSD_FindEnsoObjectByDeviceId(&deviceStatusMessage->deviceId);

            // Device announce will have different id to the gateway announce
            bool isGateway = false;
            if ( LSD_DeviceIdCompare(&deviceStatusMessage->deviceId, &_gatewayId) == 0 )
            {
                isGateway = true;
            }

            /*
             * If gateway not subscribed to Accept/Rejects topics yet do not
             * allow objects to subscribe topics.
             */
            if (!isGateway && !_bGatewayAcceptRejectTopicsSubscribed)
            {
                /*
                 * Clear "announceInProgress" flag to retry later. See
                 * _AWS_IOT_SendOutOfSyncShadows() function.
                 */
                owner->announceInProgress = false;
                AWS_StartOutOfSyncTimer();
                return;
            }

            // Get the gateway's name
            retVal = LSD_GetThingName(parentName, sizeof(parentName), &bufferUsed, _gatewayId, _gatewayId);
            if (eecNoError != retVal)
            {
                LOG_Error("LSD_GetThingName failed for gateway %d", retVal);
            }
            // Get the device's name
            if (eecNoError == retVal)
            {
                retVal = LSD_GetThingName(thingName, sizeof(thingName), &bufferUsed, deviceStatusMessage->deviceId, _gatewayId);
                if (eecNoError != retVal)
                {
                    LOG_Error("LSD_GetThingName failed for device %d", retVal);
                }
            }

            bool bNewChannel;
            int32_t connectionID;

            /* Find an available connection for new device */
            int32_t bConnectionFound = _AWS_FindAvailableConnectionForDevice(&connectionID, &bNewChannel);


            // Take these steps:
            //  - Subscribe to the device-announce-accept and device-announce-reject topics
            //  - Publish to the device-announce topic
            //  - Wait for an accept or reject
            //  - Subscribe to the delta topic

            if (eecNoError == retVal && bConnectionFound)
            {

                /*
                 * We already created first connection by AWS_Start() function
                 * so no need to create again.
                 */
                if (bNewChannel && connectionID > 0)
                {
                    /* Get channel reference */
                    AWS_CLD_CommsChannel_t* newChannel = &_awsCommsChannels[connectionID];

                    EnsoErrorCode_e retVal = newChannel->commsChannelBase.Open(&newChannel->commsChannelBase, &_gatewayId);
                    if (eecNoError != retVal)
                    {
                        LOG_Error("baseChannel->Open failed %s", LSD_EnsoErrorCode_eToString(retVal));
                        return;
                    }
                }

                if (isGateway &&
                    /* Gateway subscribes the Accept/Reject topics only one time. */
                    !_bGatewayAcceptRejectTopicsSubscribed)
                {
                    if (/*_AWS_GatewayAnnounceAndSubscriptions(parentName, owner)*/1)
                    {
                        /*
                         * Success - Now gateway subscribed to Accept/Reject topics so
                         * allow Enso Objects to subscribe too
                         */
                        _bGatewayAcceptRejectTopicsSubscribed = true;
                    }
                    else
                    {
                        // Failure - retry later
                        AWS_StartOutOfSyncTimer();
                        return;
                    }
                }

                EnsoPropertyValue_u typeVal;
                retVal = LSD_GetPropertyValueByAgentSideId(
                            &deviceStatusMessage->deviceId,
                            REPORTED_GROUP,
                            PROP_TYPE_ID,
                            &typeVal);

                if (eecNoError == retVal)
                {
#if 0
                    _AWS_AnnounceNewThing(owner,
                                          &deviceStatusMessage->deviceId,
                                          parentName,
                                          thingName,
                                          isGateway,
                                          typeVal.uint32Value,
                                          connectionID);
#endif
                }
                else // if (eecPropertyNotFound)
                {
                    LOG_Error("Child Device(%s) does not have type property! Announce is ignored!", thingName);
                }
            }
            else    /* !bConnectionfound */
            {
                LOG_Error("Unable to subscribe device.");
            }
            break;
        }

        case THING_DELETED:
        {
            /* Device is removed so unsubscribe from topics */
            retVal = _AWS_UnsubscribeDeviceID(&deviceStatusMessage->deviceId);

            if (eecNoError == retVal ||
                eecMQTTClientBusy == retVal)
            {
                /* Coninue or start to check for incomplete unsubscriptions */
                _bCheckForNotUnsubscribedObjects = true;
            }
            else
            {
                LOG_Error("Failed to unsubscribe device %llx %s",
                          deviceStatusMessage->deviceId.deviceAddress,
                          LSD_EnsoErrorCode_eToString(retVal));
            }

           // _AWS_AddToDeleteQueue(deviceStatusMessage->deviceId);
           // _AWS_KickDeleteWorker(NULL);
        }
        break;

        default:
            LOG_Error("Unexpected ECOM_THING_STATUS=%d", deviceStatusMessage->deviceStatus);
            break;
    }
}

/**
 * \brief   Consumer thread - receives messages on mq
 *
 * \param   mq Message queue on which to listen.
 */
static void _AWS_MessageQueueListener(MessageQueue_t mq)
{
    LOG_Info("Starting MessageQueueListener");

    // For ever loop
    for ( ; ; )
    {
        char buffer[ECOM_MAX_MESSAGE_SIZE];
        MessagePriority_e priority;

        int size = OSAL_ReceiveMessage(mq, buffer, sizeof(buffer), &priority);
        if (size < 1)
        {
            LOG_Error("OSAL_ReceiveMessage error %d", size);
            continue;
        }

        // Read the message id
        uint8_t messageId = buffer[0];
        switch (messageId)
        {
            case ECOM_GATEWAY_STATUS:
                if (size != sizeof(ECOM_GatewayStatus))
                {
                    LOG_Error("Invalid Gateway Status message: size is %d, expected %d", size, sizeof(ECOM_GatewayStatus));
                }
                else
                {
                    ECOM_GatewayStatus* pGWStatusMessage = (ECOM_GatewayStatus*)buffer;

                    /* Apply changes if GW Status is changed */
                    if (pGWStatusMessage->status != _bGateWayRegistered)
                    {
                        LOG_Info("GW %sregistered!", pGWStatusMessage->status == GATEWAY_REGISTERED ? "" : "un");

                        /* Keep new status */
                        _bGateWayRegistered = pGWStatusMessage->status;

                        /* Set new Online LED status. */
                        _AWS_SendLEDStatus(PROP_ONLINE_ID, _bGateWayRegistered);
                    }
                }
                break;
            case ECOM_DELTA_MSG:
                //LOG_Info("*** DELTA message received");
                if (size != sizeof(ECOM_DeltaMessage_t))
                {
                    LOG_Error("Invalid DELTA message: size is %d, expected %d", size, sizeof(ECOM_DeltaMessage_t));
                }
                else
                {
                    ECOM_DeltaMessage_t* pDeltaMessage = (ECOM_DeltaMessage_t*)buffer;
                    AWS_FaultBuffer(
                            pDeltaMessage->destinationId,
                            pDeltaMessage->deviceId,
                            pDeltaMessage->propertyGroup,
                            pDeltaMessage->numProperties,
                            pDeltaMessage->deltasBuffer);
                }
                break;

            case ECOM_PROPERTY_DELETED:
                LOG_Info("*** ECOM_PROPERTY_DELETED message received");
                if (size != sizeof(ECOM_PropertyDeletedMessage_t))
                {
                    LOG_Error("Invalid ECOM_PROPERTY_DELETED message: size is %d, expected %d", size, sizeof(ECOM_PropertyDeletedMessage_t));
                }
                else
                {
                    // Set the property to null to remove it from the thing shadow.
                    ECOM_PropertyDeletedMessage_t* pMessage = (ECOM_PropertyDeletedMessage_t*)buffer;
                    _AWS_ProcessDeletedMessage(pMessage);
                }
                break;

            case ECOM_THING_STATUS:
                //LOG_Info("*** ECOM_THING_STATUS message received");
                if (size != sizeof(ECOM_ThingStatusMessage_t))
                {
                    LOG_Error("Invalid ECOM_THING_STATUS message: size is %d, expected %d", size, sizeof(ECOM_ThingStatusMessage_t));
                }
                else
                {
                    ECOM_ThingStatusMessage_t* deviceStatusMessage  = (ECOM_ThingStatusMessage_t*)buffer;
                    _AWS_ProcessThingStatus(deviceStatusMessage);
                }
                break;

            case ECOM_LOCAL_SHADOW_STATUS:
                LOG_Info("*** ECOM_LOCAL_SHADOW_STATUS received");
                if (size != sizeof(ECOM_LocalShadowStatusMessage_t))
                {
                    LOG_Error("Invalid ECOM_LOCAL_SHADOW_STATUS message: size is %d, expected %d", size, sizeof(ECOM_LocalShadowStatusMessage_t));
                }
                else
                {
                    ECOM_LocalShadowStatusMessage_t* pMessage = (ECOM_LocalShadowStatusMessage_t*) buffer;
                    if (LSD_SYNC_COMPLETED == pMessage->shadowStatus)
                    {
                        LOG_Info("Local Shadow : LSD_SYNC_COMPLETED for property group %d", pMessage->propertyGroup);
                    }
                    else
                    {
                        LOG_Error("Unexpected Local Shadow status: %d", pMessage->shadowStatus);
                    }
                }
                break;

            default:
                LOG_Error("Unknown message %d", messageId);
                break;
        }
    }

    LOG_Warning("MessageQueueListener exit");
}


/*
 * \brief Handler function for processing property change deltas
 *
 * \param  subscriberId      The destination id to receive the notification
 *
 * \param  deviceId          The thing that changed so is publishing the
 *                           update to its subscriber list.
 *
 * \param  propertyGroup     The group to which the properties belong (reported
 *                           or desired)
 *
 * \param  numProperties     The number of properties in the delta
 *
 * \param  deltasBuffer      The list of properties that have been changed
 *
 * \param  context           context assocated with this delta for callback
 *
 * \return                   Error code
 */
EnsoErrorCode_e AWS_OnCommsHandler(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t deviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer,
        void* context)
{
#if 0
    // Sanity check
    assert(deltasBuffer);
    assert(COMMS_HANDLER == subscriberId);

    // Consume ECOM message or the mq will fill up

    char listParents[ECOM_MAX_DELTAS][LSD_SIMPLE_PROPERTY_NAME_MAX_LENGTH+1];
    int listParentIndex = 0;

    /* Always use channel 0 to send deltas */
    AWS_CLD_CommsChannel_t* channel0 = &_awsCommsChannels[0];

    EnsoErrorCode_e retVal = channel0->commsChannelBase.DeltaStart(&channel0->commsChannelBase, propertyGroup);

    if (retVal != eecNoError)
    {
        LOG_Error("Delta start error %d", retVal);
    }

    // Do all unnested properties first
    for (int i = 0; (eecNoError == retVal) && (i < numProperties); i++)
    {
        EnsoProperty_t* changedProperty = LSD_GetPropertyByAgentSideId( &deviceId, deltasBuffer[i].agentSidePropertyID);
        if (changedProperty)
        {
            char parentName[LSD_PROPERTY_NAME_BUFFER_SIZE];
            char childName[LSD_PROPERTY_NAME_BUFFER_SIZE];
            bool nestedProperty = LSD_IsPropertyNested(changedProperty->cloudName, parentName, childName);

            if (!nestedProperty)
            {
                retVal = channel0->commsChannelBase.DeltaAddProperty(&channel0->commsChannelBase,
                    changedProperty, false, propertyGroup);
            }

            if (eecNoError != retVal)
            {
                /* This is now a warning rather than an error because there is an issue with
                 * converting large memory blob properties to JSON as the AWS maximum buffer size
                 * for a JSON is 400 bytes. The properties in question are expected to be private
                 * and not subscribed to so there should be no delta sent to AWS when
                 * AWS_CommsChannelDeltaSend is invoked. Ideally the conversion to JSON should only be
                 * done on property deltas which are actually transmitted to AWS but this is
                 * a bigger change and will not be done at this stage.
                 *
                 * Currently (20/02/18) the only property affected is the devicesData (agentsideID 0x40000013)
                 * for which a private property memory blob containing a JSON for the saved devices data is
                 * set.
                 */
                LOG_Warning("Delta Add property: %s agentSideID: %08x returned %d", changedProperty->cloudName, deltasBuffer[i].agentSidePropertyID, retVal);
                // Reset error
                retVal = eecNoError;
            }
        }
        else
        {
            LOG_Error("Failed to find property %08x", deltasBuffer[i].agentSidePropertyID);
        }
    }

    // Do nested properties second
    for (int i = 0; (eecNoError == retVal) && (i < numProperties); i++)
    {
        EnsoProperty_t* parentProperty = LSD_GetPropertyByAgentSideId(&deviceId, deltasBuffer[i].agentSidePropertyID);

        if (parentProperty)
        {
            char parentNameI[LSD_PROPERTY_NAME_BUFFER_SIZE];
            char childName[LSD_PROPERTY_NAME_BUFFER_SIZE];
            bool nestedProperty = LSD_IsPropertyNested(parentProperty->cloudName, parentNameI, childName);

            if (nestedProperty)
            {
                // Have we already done this parent group?
                bool foundParent = false;
                for (int f=0; f<listParentIndex; f++)
                {
                    if (strcmp(listParents[f], parentNameI) == 0)
                    {
                        // We have already seen this parent
                        foundParent = true;
                    }
                }

                if (foundParent)
                {
                    continue;
                }

                if (listParentIndex < ECOM_MAX_DELTAS)
                {
                    // New parent, add this parent to the list
                    snprintf(listParents[listParentIndex], LSD_SIMPLE_PROPERTY_NAME_MAX_LENGTH+1, "%s", parentNameI);
                    listParentIndex++;
                }
                else
                {
                    LOG_Error("Exceeded maximum number of nested properties for an update");
                    retVal = eecInternalError;
                    break;
                }

                bool firstTime = true;
                // Loop through all properties and add all that have same parent in one go
                for (int j = i; (eecNoError == retVal) && (j < numProperties); j++)
                {
                    EnsoProperty_t* changedProperty = LSD_GetPropertyByAgentSideId(&deviceId, deltasBuffer[j].agentSidePropertyID);
                    if (changedProperty)
                    {
                        char parentNameJ[LSD_PROPERTY_NAME_BUFFER_SIZE];
                        nestedProperty = LSD_IsPropertyNested(changedProperty->cloudName, parentNameJ, childName);

                        // If parent names match then we should add to this set
                        if (nestedProperty && (strcmp(parentNameI, parentNameJ) == 0))
                        {
                            // Add parent name group first time through only
                            retVal = channel0->commsChannelBase.DeltaAddProperty(&channel0->commsChannelBase,
                                 changedProperty, firstTime, propertyGroup);
                            firstTime = false;

                            if (eecNoError != retVal)
                            {
                                LOG_Error("Add nested property %08x error %d", deltasBuffer[j].agentSidePropertyID, retVal);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            LOG_Error("Failed to find property %08x", deltasBuffer[i].agentSidePropertyID);
        }
    }

    if (eecNoError == retVal)
    {
        retVal = channel0->commsChannelBase.DeltaSend(&channel0->commsChannelBase,
                deviceId, context);
        if (eecNoError == retVal)
        {
            retVal = channel0->commsChannelBase.DeltaFinish(&channel0->commsChannelBase);
        }
        else
        {
            LOG_Error("Send delta error %s", LSD_EnsoErrorCode_eToString(retVal));
        }
    }
#endif
    return 0;//retVal;
}

/*
 * \brief Parse a JSON value pair to update a property value.
 *
 * \param  rxBuf             The buffer holding the raw json data
 *
 * \param  jsmntokp          The structure describing the json tokens
 *
 * \param  deviceId          The device ID of the thing to which the property belongs to
 *
 * \param  propName          The property name
 *
 * \param  jsonTokenIndex    The value index in the JSON token structure
 *
 * \return  Error code
 */
#if 0
static EnsoErrorCode_e _AWS_ParseJsonValue(
        char       * rxBuf,
     //   jsmntok_t  * jsmntokp,
        EnsoDeviceId_t* deviceId,
        const char * propName,
        int          jsonTokenIndex)
{
    assert(propName);
    EnsoErrorCode_e  ensoret   = eecNoError;
    EnsoProperty_t * property  = LSD_GetPropertyByCloudName(deviceId, propName);

    jsmntok_t        jsmntok   = jsmntokp[jsonTokenIndex];
    int              size      = jsmntok.end - jsmntok.start + 1;
    jsmntype_t       jsmntype  = jsmntok.type;
    IoT_Error_t      iotret    = 0;
#endif
    EnsoPropertyValue_u value;
#if 0
    if (property == NULL)
    {
        ensoret = eecPropertyNotFound;
    }

    else
    {

    	EnsoValueType_e  valueType = property->type.valueType;

        switch (jsmntype)
        {

        case JSMN_UNDEFINED:
        case JSMN_OBJECT:
        case JSMN_ARRAY:
            ensoret = eecConversionFailed;
            break;
        case JSMN_STRING:
            {
                char str[size];
                iotret = _AWS_ParseStringValue(str, size, rxBuf, &jsmntok);
                LOG_Trace("%s:\"%s\"", propName, str);
                if (iotret == 0)
                {
                    switch (valueType)
                    {
                    case evBlobHandle:
                        {
                            int blobsize = strlen(str) + 1;
                            value.memoryHandle = OSAL_MemoryRequest(NULL, blobsize);
                            if (value.memoryHandle == NULL)
                            {
                                LOG_Error("NULL memoryHandle");
                                ensoret = eecInternalError;
                            }
                            else
                            {
                                strcpy(value.memoryHandle, str);
                                LOG_Trace("Blob:%d:\"%s\"", blobsize, value.memoryHandle ? value.memoryHandle : "");
                            }
                        }
                        break;
                    case evString:
                        if (size <= sizeof value.stringValue)
                        {
                            strcpy(value.stringValue, str);
                        }
                        else
                        {
                            ensoret = eecBufferTooSmall;
                        }
                        break;
                    default:
                        LOG_Error("Not Blob or String");
                        ensoret = eecInternalError;
                    }
                }
            }
            break;
        case JSMN_PRIMITIVE:
            iotret = _AWS_ParsePrimitive(valueType, &value, rxBuf, jsmntok);
            break;
        default:
            LOG_Error("Invalid JSON type");
            ensoret = eecInternalError;
        }

        if (iotret != 0)
        {
            ensoret = eecConversionFailed;
        }
        else if (ensoret == eecNoError)
        {
            char buf[12];
            LOG_Trace("%s %s \"%s\"", propName, LSD_ValueType2s(valueType), LSD_Value2s(buf, sizeof buf, valueType, &value));
            EnsoPropertyDelta_t dummyDelta;
            int dummyCounter = 0;
            ensoret = LSD_SetPropertyValueByCloudNameWithoutNotification(COMMS_HANDLER,
                    deviceId, DESIRED_GROUP, propName, value, &dummyDelta, &dummyCounter);
        }
    }

    return 0;
}
#endif
/*
 * \brief Parse a JSON structure recursively.
 *
 * \param notifyOnly        False for first pass, true for second pass.
 *
 * \param deviceId          The device ID of the thing to which the property belongs to.
 *
 * \param shadowRxBuf       The buffer holding the raw json data.
 *
 * \param jsonTokenStruct   The structure describing the json tokens.
 *
 * \param parentName        Name of parent element, or NULL if root.
 *
 * \param current           The index of the start element.
 *
 * \param end               The index of the last element.
 *
 * \param tokenCount        Total number of tokens in the JSON.
 *
 * \param deltaBuffer       The buffer of deltas that we are building up.
 *
 * \param deltaCounter      The current position in the delta buffer.
 *
 * \param notify            Who to notify with deltas.
 *
 * \return Error code
 *
 */
#if 0
static EnsoErrorCode_e _AWS_ProcessJSONRecursively(bool notifyOnly, EnsoDeviceId_t* deviceId, char* shadowRxBuf, jsmntok_t* jsonTokenStruct, char* parentName, int* current, int end, int tokenCount, EnsoPropertyDelta_t* deltaBuffer, int* deltaCounter, EnsoObject_t* notify)
{
    EnsoErrorCode_e result = eecNoError;

    /* We have two modes of operation controlled by 'notifyOnly':

       notifyOnly = false: This is the first pass where we parse the
       JSON and write values to the local shadow, but we don't build
       up the delta buffer, that is done in the second pass.

       notifyOnly = true: We don't update the shadow (that will have
       been done by the first pass), but we do build up the delta
       buffer and send if/when it becomes full.
    */
    if (notifyOnly == false)
    {
        assert(deltaBuffer == NULL);
        assert(deltaCounter == NULL);
        assert(notify == NULL);
    }

    if (notifyOnly == true)
    {
        assert(deltaBuffer != NULL);
        assert(deltaCounter != NULL);
        assert(notify != NULL);
    }

    /* Process all elements in the section of JSON that we have been passed. */
    while ((*current < tokenCount) && (jsonTokenStruct[*current].start <= end))
    {
        /* Parse the key of the next token to process. */
        char keyName[LSD_PROPERTY_NAME_BUFFER_SIZE];
        result = _AWS_ParseStringValue(keyName, LSD_PROPERTY_NAME_BUFFER_SIZE, shadowRxBuf, &jsonTokenStruct[*current]);
        if (eecNoError != result)
        {
            LOG_Error("parseStringValue failed %s", LSD_EnsoErrorCode_eToString(result));
            break;
        }

        /* See if 'no change' has been set on this property by an earlier pass. */
        bool noChange = (jsonTokenStruct[(*current) + 1].type & JSMN_NO_CHANGE);
        jsonTokenStruct[(*current) + 1].type &= ~JSMN_NO_CHANGE; // Clear the flag now that we've read it.

        /* See if the element is a regular element, or a nested element. */
        if (JSMN_OBJECT == jsonTokenStruct[(*current) + 1].type)
        {
            /* It's a nested JSON object so process it recursively. */
            int nestedEnd = jsonTokenStruct[(*current) + 1].end;
            *current += 2;
            result = _AWS_ProcessJSONRecursively(notifyOnly, deviceId, shadowRxBuf, jsonTokenStruct, keyName, current, nestedEnd, tokenCount, deltaBuffer, deltaCounter, notify);
        }
        else if ((JSMN_PRIMITIVE == jsonTokenStruct[(*current) + 1].type) || (JSMN_STRING == jsonTokenStruct[(*current) + 1].type))
        {
            /* It's a regular single element. Get the property name including any parent component. */
            char propName[LSD_PROPERTY_NAME_BUFFER_SIZE];
            if (parentName != NULL)
            {
                snprintf(propName, LSD_PROPERTY_NAME_BUFFER_SIZE, "%s_%s", parentName, keyName);
            }
            else
            {
                snprintf(propName, LSD_PROPERTY_NAME_BUFFER_SIZE, "%s", keyName);
            }

            /* Are we in the first pass, or the second pass? */
            if (notifyOnly && !noChange)
            {
                /* In this pass the value has already been processed
                 * an is in the local shadow. All we have to do is
                 * read back the value and add it to the delta
                 * buffer. */
                EnsoProperty_t* property = LSD_GetPropertyByCloudName(deviceId, propName);
                if (property != NULL)
                {
                    deltaBuffer[*deltaCounter].propertyValue = property->desiredValue;
                    deltaBuffer[*deltaCounter].agentSidePropertyID = property->agentSidePropertyID;
                    *deltaCounter += 1;
                }
                else
                {
                    LOG_Error("JSON element parser failed to read back property value for %s.", propName);
                }
            }
            else if (!notifyOnly)
            {
                /* In this (first) pass, we need to parse the JSON and
                 * write the value to the local shadow - but don't
                 * send any delta notifications just yet. */
                result = _AWS_ParseJsonValue(shadowRxBuf, jsonTokenStruct, deviceId, propName, ((*current) + 1));

                /* Remember if there is "no change" so that the second
                 * pass can avoid sending an unnecessay delta. We do
                 * this by overloading the type field. */
                if (result == eecNoChange)
                {
                    result = eecNoError;
                    jsonTokenStruct[(*current) + 1].type |= JSMN_NO_CHANGE;
                }
                else if (result == eecPropertyNotFound)
                {
                    /* Delta received for a property which we don't have locally, just ignore. */
                    result = eecNoError;
                    jsonTokenStruct[(*current) + 1].type |= JSMN_NO_CHANGE;
                }
            }

            /* On to the next element. */
            *current += 2;
        }
        else
        {
            /* Unknown element type. */
            LOG_Error("Unexpected json type %d", jsonTokenStruct[(*current) + 1].type);
            result = eecConversionFailed;
        }

        /* Was there an error above? */
        if (result != eecNoError)
        {
            LOG_Error("JSON element parsing failed for %s%s%s: %s",
                parentName ? parentName : "", parentName ? "_" : "",
                keyName, LSD_EnsoErrorCode_eToString(result));
            break;
        }

        /* Have we been asked to populate a delta buffer, if so see if it is full? */
        if ((deltaBuffer != NULL) && (*deltaCounter >= ECOM_MAX_DELTAS))
        {
            /* Warn if there are going to be multiple delta buffers, ie. an atomic set of desired changes don't fit in a single delta buffer. */
            if ((*current < tokenCount) && (jsonTokenStruct[*current].start <= end))
            {
                LOG_Info("Atomic set of desired deltas straddled multiple messages.");
            }

            /* Send the current delta buffer. */
            assert(notify != NULL);
            EnsoErrorCode_e result = LSD_NotifyDirectly(COMMS_HANDLER, notify, DESIRED_GROUP, deltaBuffer, *deltaCounter);
            if (result != eecNoError)
            {
                LOG_Error("Failed to send delta buffer.");
                break;
            }

            /* Prepare a new empty delta buffer. */
            *deltaCounter = 0;
        }
    }

    return result;
}
#endif
/*
 * \brief The callback function called when a delta is received on the update/delta topic
 *
 * \param  pClient      Unused
 *
 * \param  topicName    Name of the topic that was subscribed to
 *
 * \param  topicNameLen Unused
 *
 * \param  params       MQTT Publish Message Parameters
 *
 * \param  pData        Unused
 *
 */
#if 0
void _AWS_ShadowDeltaCallback(
        AWS_IoT_Client *pClient,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData)
{
    // Sanity check
    if (!topicName || !params)
    {
        LOG_Error("Null pointer");
        return;
    }
    if (params->payloadLen > SHADOW_MAX_SIZE_OF_RX_BUFFER)
    {
        LOG_Error("Payload larger than RX Buffer");
        return;
    }

    (void)pClient;      // Unused
    (void)topicNameLen; // Unused
    (void)pData;        // Unused

    LOG_Trace("%s", topicName);

    char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];

    int parsed;
    if (topicName[36] == '_')
    {
        parsed = sscanf(topicName, "$aws/things/%49[^'/']/%*s", thingName);
    }
    else
    {
        parsed = sscanf(topicName, "$aws/things/%24[^'/']/%*s", thingName);
    }
    if (parsed != 1)
    {
        LOG_Error("sscanf failed, cannot read thing name");
        return;
    }

    int32_t connectionID;
    EnsoErrorCode_e retVal = _AWS_GetConnectionIDForThingName(thingName, &connectionID);

    if (eecNoError != retVal)
    {
        LOG_Error("_AWS_GetConnectionAndSubscribtionIDFromThingName error %s", LSD_EnsoErrorCode_eToString(retVal));
        return;
    }

    char* shadowRxBuf = _awsCommsChannels[connectionID].shadowRxBuf;
    jsmntok_t* jsonTokenStruct = _awsCommsChannels[connectionID].jsonTokenStruct;

    memcpy(shadowRxBuf, params->payload, params->payloadLen);
    shadowRxBuf[params->payloadLen] = '\0';    // jsmn_parse relies on a string
    for (int i = 0; i < params->payloadLen; i += 100)
    {
        LOG_Info("len %d:\"%-.100s\"", params->payloadLen, shadowRxBuf+i);
    }

    jsmn_parser shadowJsonParser;
    jsmn_init(&shadowJsonParser);

    int32_t tokenCount = jsmn_parse(&shadowJsonParser, shadowRxBuf, strlen(shadowRxBuf), jsonTokenStruct,
            MAX_JSON_TOKEN_EXPECTED);

    if (tokenCount < 0)
    {
        LOG_Warning("Failed to parse JSON: %d", tokenCount);
        return;
    }

    /* Assume the top-level element is an object */
    if (tokenCount < 1 || jsonTokenStruct[0].type != JSMN_OBJECT)
    {
        LOG_Warning("Top Level is not an object\n");
        return;
    }

    //for (unsigned int i = 1; i < tokenCount; i++)
    //{
    //    jsmntok_t dataToken = _jsonTokenStruct[i];
    //    LOG_Info("%d: start %d end %d size %d type %d", i, dataToken.start, dataToken.end, dataToken.size, dataToken.type);
    //}

    void * pJsonHandler = (void *) jsonTokenStruct;

    uint32_t tokenIndex;

    // Find the "state" key
    if (!_AWS_IsJsonKeyMatching(shadowRxBuf, pJsonHandler, tokenCount,
            "state", &tokenIndex))
    {
        LOG_Error("Did not find \"state\" key");
        return;
    }

    /*
     * First we have to parse all the JSON and put the corresponding updates in the shadow, but don't pass on deltas to subscribers - do that in a second pass. *
     */
    EnsoDeviceId_t deviceId;
    retVal = LSD_GetThingFromNameString(thingName, &deviceId);
    if (retVal == eecNoError)
    {
        int current = tokenIndex + 2; // Start of the first key-value pair
        int stateEnd = jsonTokenStruct[tokenIndex + 1].end; // Next index is the JSMN_OBJECT
        retVal = _AWS_ProcessJSONRecursively(false, &deviceId, shadowRxBuf, jsonTokenStruct, NULL, &current, stateEnd, tokenCount, NULL, NULL, NULL);

        /*
         * Now process the JSON again but this time do pass on the deltas to subscribers.
         */
        if (retVal == eecNoError)
        {
            EnsoObject_t* notify = LSD_FindEnsoObjectByDeviceIdDirectly(&deviceId);
            if (notify)
            {
                EnsoPropertyDelta_t deltaBuffer[ECOM_MAX_DELTAS];
                int deltaCounter = 0;
                current = tokenIndex + 2; // Start of the first key-value pair
                stateEnd = jsonTokenStruct[tokenIndex + 1].end; // Next index is the JSMN_OBJECT
                retVal = _AWS_ProcessJSONRecursively(true, &deviceId, shadowRxBuf, jsonTokenStruct, NULL, &current, stateEnd, tokenCount, &(deltaBuffer[0]), &deltaCounter, notify);
                if (retVal != eecNoError)
                {
                    LOG_Error("Failed to parse JSON and send delta notification, error=%s.", LSD_EnsoErrorCode_eToString(retVal));
                }
                /* See if there are any residual deltas to be notified? */
                else if (deltaCounter > 0)
                {
                    retVal = LSD_NotifyDirectly(COMMS_HANDLER, notify, DESIRED_GROUP, deltaBuffer, deltaCounter);
                    if (retVal != eecNoError)
                    {
                        LOG_Error("Failed to send delta buffer.");
                    }
                }
            }
            else
            {
                LOG_Error("Could not find enso object for device.");
            }
        }
        else
        {
            LOG_Warning("Skipping delta notifications because initial parsing failed, error=%s.", LSD_EnsoErrorCode_eToString(retVal));
        }
    }
    else
    {
        LOG_Error("Could not get deivce ID for thing=%s, error=%s.", thingName, LSD_EnsoErrorCode_eToString(retVal));
    }
}
#endif
/*
 * \brief Update the status of a thing
 *
 * \param  thingName      the name of the thing
 *
 * \param  status         New status of the thing
 *
 * \return                Error code
 */
#if 0
static EnsoErrorCode_e _AWS_UpdateThingStatus(
        const char *thingName,
        EnsoDeviceStatus_e status)
{
    EnsoDeviceId_t theDevice;
    EnsoErrorCode_e retVal = LSD_GetThingFromNameString(thingName, &theDevice);
    if (eecNoError == retVal)
    {
        retVal = LSD_SetDeviceStatus(theDevice, status);
    }
    else
    {
        LOG_Error("LSD_GetThingFromNameString failed %s", LSD_EnsoErrorCode_eToString(retVal));
    }
    return retVal;
}

/*
 * \brief The callback function called when device announce is accepted
 *
 * \param  pClient      Unused
 *
 * \param  topicName    Name of the topic that was subscribed to
 *
 * \param  topicNameLen Topic name length
 *
 * \param  params       MQTT Publish Message Parameters
 *
 * \param  pData        Unused
 *
 */
void _AWS_AnnounceAccept(
       // AWS_IoT_Client *unused,
        char *topicName,
        uint16_t topicNameLen,
        //IoT_Publish_Message_Params *params,
        void *pData)
{
    (void)unused;

    // Sanity check
    if (!topicName || !params)
    {
        LOG_Error("Null pointer");
        return;
    }

    char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];
    int parsed;
    if (((char*)params->payload)[38] == '_')
    {
        parsed = sscanf(params->payload, "{ \"deviceId\": \"%49[^'/']\"%*s", thingName);
    }
    else
    {
        parsed = sscanf(params->payload, "{ \"deviceId\": \"%24[^'/']\"%*s", thingName);
    }
    if (parsed != 1)
    {
        LOG_Error("sscanf failed, cannot read thing name");
        return;
    }

    EnsoDeviceId_t deviceID;
    LSD_GetThingFromNameString(thingName, &deviceID);

    if (_bGatewayAnnounceAccepted == true && LSD_DeviceIdCompare(&deviceID, &_gatewayId) == 0 )
    {
        LOG_Info("Announce by different device. Ignored.");
        return;
    }

    LOG_Info("Announce Accept: %*.*s", topicNameLen, topicNameLen, topicName);
    LOG_Info("Announce accept for %s", thingName);

    _bGatewayAnnounceAccepted = true;

    EnsoErrorCode_e retVal = _AWS_UpdateThingStatus(thingName, THING_ACCEPTED);
    if (eecNoError != retVal)
    {
        LOG_Error("Failed to update %s status %s", thingName, LSD_EnsoErrorCode_eToString(retVal));
        return;
    }

    int32_t connectionID;
    retVal = _AWS_GetConnectionIDForThingName(thingName, &connectionID);
    if (eecNoError != retVal || connectionID < 0 || connectionID >= MAX_CONNECTION_PER_GATEWAY)
    {
        LOG_Error("Failed to get connection id of device %s", thingName);
        return;
    }

    AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[connectionID];

    /* Find an empty slot for delta topic of new device */
    int32_t emptySlotIndex = _AWS_FindEmptySlotForDeltaTopic(channel);

    if (emptySlotIndex < 0)
    {
        /* This should not happen since it is checked when allocating a connection id to the device */
        LOG_Error("There is not empty slot for delta topic!");
        return;
    }

    char* awsShadowUpdateDeltaTopic = channel->awsShadowUpdateDeltaTopic[emptySlotIndex];

    // Subscribe to the delta topic
    snprintf(awsShadowUpdateDeltaTopic,
             MAX_SHADOW_TOPIC_LENGTH_BYTES,
             "$aws/things/%s/shadow/update/delta", thingName);

    LOG_Info("delta topic for %s %s", thingName, awsShadowUpdateDeltaTopic);

    SeqQueueItem item;
    item.type = SeqItem_Subscribe;
    item.args.Subscribe.channelID = connectionID;
    item.args.Subscribe.slotIndex = emptySlotIndex;

    EnsoDeviceId_t deviceId;
    retVal = LSD_GetThingFromNameString(thingName, &deviceId);
    EnsoObject_t* owner = LSD_FindEnsoObjectByDeviceId(&deviceId);

    bool bPush = _AWS_QueueSeqItem(item, SeqItemPri_Low);
    if (bPush == false)
    {
        LOG_Error("Failed to add Sequencer Queue!");

        _commsDelayMs = aws_out_of_sync_initial_shadow_recovery_interval_ms;

        /*
         * MQTT subscription is failed, one of the reason can be that AWS does
         * not know the device cert yet for the first time. In this case, we
         * AWS gets device cert in first attemption and accepts device in second
         * attemp.
         *
         * Therefore, we need to announce device again.
         */
        owner->announceAccepted = false;
        owner->announceInProgress = false;

        /*
         * Subscription count was increased and will be increased for the next
         * time so we need to decrease it.
         */
        channel->subscriptionCount--;

        /* Clean update delta topic for fail.  */
        memset(awsShadowUpdateDeltaTopic, 0, MAX_SHADOW_TOPIC_LENGTH_BYTES);
    }
    else
    {
        LOG_Info("Subscribed to %s", awsShadowUpdateDeltaTopic);

        owner->announceAccepted = true;

        /* Now that we've had a successful announce, we can use the short recovery time interval for subsequent announces. */
        _commsDelayMs = aws_out_of_sync_short_shadow_recovery_interval_ms;

        /*
         * Initial properties of an accepted device are market as out-of-sync
         * so we need to sync them with cloud
         */
        AWS_StartOutOfSyncTimer();
    }
}

/*
 * \brief The callback function called when device announce is rejected
 *
 * \param  pClient      Unused
 *
 * \param  topicName    Name of the topic that was subscribed to
 *
 * \param  topicNameLen Topic name length
 *
 * \param  params       MQTT Publish Message Parameters
 *
 * \param  pData        Unused
 *
 */
void _AWS_AnnounceReject(
        AWS_IoT_Client *pClient,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData)
{
    // Sanity check
    if (!pClient || !topicName || !params)
    {
        LOG_Error("Null pointer");
        return;
    }

    LOG_Warning("Device has failed to register, is it registered to another gateway?");

    char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];

    int parsed;
    if (((char*)params->payload)[38] == '_')
    {
        parsed = sscanf(params->payload, "{ \"deviceId\": \"%49[^'/']\"%*s", thingName);
    }
    else
    {
        parsed = sscanf(params->payload, "{ \"deviceId\": \"%24[^'/']\"%*s", thingName);
    }
    if (parsed != 1)
    {
        LOG_Error("sscanf failed, cannot read thing name");
        return;
    }

    LOG_Warning("Announce Reject: %s", thingName);

    EnsoErrorCode_e retVal = _AWS_UpdateThingStatus(thingName, THING_REJECTED);

    if (eecNoError != retVal)
    {
        LOG_Error("Update thing status failed: %i", retVal);
    }

    int32_t connectionID;

    retVal = _AWS_GetConnectionIDForThingName(thingName, &connectionID);

    if (eecNoError != retVal)
    {
        LOG_Error("Failed to get connection ID %s", LSD_EnsoErrorCode_eToString(retVal));
        return;
    }

    /* Remove subscription from connection */
    _awsCommsChannels[connectionID].subscriptionCount--;
}

/*
 * \brief The callback function called when device shadow is deleted
 *
 * \param  pClient      Unused
 * \param  topicName    Unused
 * \param  topicNameLen Unused
 * \param  params       Unused
 * \param  pData        Unused
 */
static void _AWS_GWDeleteAccept(
        AWS_IoT_Client *pClient,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData)
{
    // Sanity check
    if (!pClient || !topicName || !params)
    {
        LOG_Error("Null pointer");
        return;
    }
    LOG_Warning("Gateway Shadow is deleted, removing logs and rebooting");
    /* Apply changes if GW Status is changed */
    if (false != _bGateWayRegistered)
    {
        /* Keep new status */
        _bGateWayRegistered = false;

        /* Set new Online LED status. */
        _AWS_SendLEDStatus(PROP_ONLINE_ID, _bGateWayRegistered);
    }
    STO_RemoveLogs();
    OSAL_sleep_ms(10000);
    HAL_Reboot();
}

/*
 * \brief The callback function called when a device is cancelled
 *
 * \param  pClient      Unused
 * \param  topicName    Unused
 * \param  topicNameLen Unused
 * \param  params       Unused
 * \param  pData        Unused
 */
static void _AWS_CancelAccept(
        AWS_IoT_Client *pClient,
        char *topicName,
        uint16_t topicNameLen,
        IoT_Publish_Message_Params *params,
        void *pData)
{
    // Sanity check
    if (!pClient || !topicName || !params)
    {
        LOG_Error("Null pointer");
        return;
    }
    char deviceName[ENSO_OBJECT_NAME_BUFFER_SIZE];

    if (KVP_GetString("deviceId", params->payload+1, deviceName, ENSO_OBJECT_NAME_BUFFER_SIZE))
    {
        LOG_Info("Device %s cancel confirmed", deviceName);

        _AWS_RemoveFromDeleteQueue(deviceName);

        // We can process another one now if there are any in the queue
        _AWS_KickDeleteWorker(NULL);
    }
    else
    {
        LOG_Error("Failed to find deviceId in payload: %s", params->payload);
    }
}

/*
 * \brief Iterates through the JSON tokens to find a matching key
 *
 * \param  pJsonDocument     The JSON document
 *
 * \param  pJsonHandler      The parsed JSON token structures
 *
 * \param  tokenCount        The number of parsed tokens
 *
 * \param  pKey              The key to match
 *
 * \param  pDataIndex        The key index in the token structure
 *
 * \return                   True if a match is found, false otherwise
 */
static bool _AWS_IsJsonKeyMatching(
        const char *pJsonDocument,
        void *pJsonHandler,
        int32_t tokenCount,
        const char *pKey,
        uint32_t *pDataIndex)
{
    jsmntok_t* pjsonTokenStruct = (jsmntok_t*) pJsonHandler;
    pjsonTokenStruct++;
    for (unsigned int i = 1; i < tokenCount; i++)
    {
        if (jsoneq(pJsonDocument, pjsonTokenStruct, pKey) == 0)
        {
            *pDataIndex = i;
            return true;
        }
        pjsonTokenStruct++;
    }
    return false;
}

/*
 * \brief Parses a JSON primitive
 *
 * \param       type         The type of the value to parse
 *
 * \param[out]  pValue       The parsed value
 *
 * \param       pJsonString  The JSON string
 *
 * \param       token        The JSON token
 *
 * \return                   Error code
 */
static IoT_Error_t _AWS_ParsePrimitive(
        EnsoValueType_e type,
        EnsoPropertyValue_u *pValue,
        const char *pJsonString,
        jsmntok_t token)
{
    IoT_Error_t ret_val = SUCCESS;
    if (type == evBoolean)
    {
        ret_val = parseBooleanValue(&pValue->booleanValue, pJsonString, &token);
    }
    else if (type == evInt32)
    {
        ret_val = parseInteger32Value(&pValue->int32Value, pJsonString, &token);
    }
    else if (type == evTimestamp)
    {
        pValue->timestamp.isValid = true; // It's a desired value so it will always be valid
        ret_val = parseInteger32Value(&pValue->int32Value, pJsonString, &token);
    }
    else if (type == evUnsignedInt32)
    {
        ret_val = parseUnsignedInteger32Value(&pValue->uint32Value, pJsonString, &token);
    }
    else if (type == evFloat32)
    {
        ret_val = parseFloatValue(&pValue->float32Value, pJsonString, &token);
    }
    else
    {
        ret_val = JSON_PARSE_ERROR;
    }

    return ret_val;
}

/*
 * \brief Callback invoked upon MQTT connection loss
 *
 */
static void _AWS_DisconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
    /* Close Online led */
    _AWS_SendLEDStatus(PROP_ONLINE_ID, false);

    if (!pClient)
    {
        return;
    }

    (void)data;

    /* Channel is disconnected so we need to find its reference and mark disconnected */
    int32_t communicationID;
    for (communicationID = 0; communicationID < MAX_CONNECTION_PER_GATEWAY; communicationID++)
    {
        if (&_awsCommsChannels[communicationID].mqttClient == pClient)
        {
            /* Communication ID is found  */
            break;
        }
    }

    if (MAX_CONNECTION_PER_GATEWAY == communicationID)
    {
        LOG_Error("Client does not exist.");
    }
    else
    {
        LOG_Warning("MQTT Disconnect on channel %d", communicationID);

        AWS_CLD_CommsChannel_t* channel = &_awsCommsChannels[communicationID];
        /* Set connection status of channel as disconnected */
        channel->bConnected = false;

        /* Disconnect on any channel stops our updates so reset message sending */
        AWS_SendFailedForFaultBuffer();

        /* We need to set Online Property for channel 0 */
        if (0 == communicationID)
        {
            EnsoPropertyValue_u value = { 0 };
            value.uint32Value = 0;

            /*
             * All updates are sent over Channel 0 so just inform Fault Buffers
             * about new state for channel 0
             * */
            AWS_ConnectionStateCB(false);

            // Set the online property for gateway connection which is in connection 0
            EnsoErrorCode_e retVal =  LSD_SetPropertyValueByAgentSideId(
                                        COMMS_HANDLER,
                                        &_gatewayId,
                                        REPORTED_GROUP,
                                        PROP_ONLINE_ID,
                                        value);
            if (eecNoError != retVal)
            {
                LOG_Error("Failed to set Gateway Online Property %s", LSD_EnsoErrorCode_eToString(retVal));
            }
        }

        /* Start to exponential back-off by minimum value */
        _connRetryTimeout = AWS_CONNECTION_MIN_RECONNECT_WAIT_INTERVAL_MS;

        /* Connection is broken so set one-shot timer to retry again  */
        if (channel->connectTimer)
        {
            OSAL_DestroyTimer(channel->connectTimer);
            channel->connectTimer = NULL;
        }
        channel->connectTimer = OSAL_NewTimer(_AWS_IOT_ConnectTimerCB, _connRetryTimeout, false, channel);

        if (NULL == channel->connectTimer)
        {
            LOG_Error("Could not create AWS Connect timer.");
        }
    }
}

/*
 * \brief Parses a JSON string, unescaping any escaped characters
 *
 * \param       buf          The destination buffer
 *
 * \param       bufSize      Size of destination buffer in bytes
 *
 * \param       jsonString   The raw json
 *
 * \param       token        Structure holding our token information
 *
 * \return                   Error code or Success
 */
EnsoErrorCode_e _AWS_ParseStringValue(char *buf, int bufSize, const char *jsonString,
        jsmntok_t *token)
{
    EnsoErrorCode_e result = eecNoError;

    if(token->type != JSMN_STRING)
    {
        LOG_Warning("Token was not a string.");
        return JSON_PARSE_ERROR;
    }

    int jsonIndex = token->start;
    int bufIndex = 0;

    while ((result == eecNoError) && (jsonIndex < token->end) && (bufIndex < bufSize))
    {
        if (jsonString[jsonIndex] == '\\')
        {
            switch(jsonString[++jsonIndex])
            {
                case 'b':
                    buf[bufIndex++]='\b';
                    break;
                case 'f':
                    buf[bufIndex++]='\f';
                    break;
                case 'n':
                    buf[bufIndex++]='\n';
                    break;
                case 'r':
                    buf[bufIndex++]='\r';
                    break;
                case 't':
                    buf[bufIndex++]='\t';
                    break;
                case '"':
                    buf[bufIndex++]='\"';
                    break;
                case '\\':
                    buf[bufIndex++]='\\';
                    break;
                default:
                    LOG_Error("Unknown escape code %c", jsonString[jsonIndex-1]);
                    result = eecConversionFailed;
                    break;
            }

            jsonIndex++;
        }
        else
        {
            buf[bufIndex++] = jsonString[jsonIndex++];
        }
    }

    if (bufIndex < bufSize)
    {
        buf[bufIndex] = '\0';
    }
    else
    {
        result = eecBufferTooSmall;
    }

    return result;
}


/*
 * \brief  Check JSON values against actual values in shadow and clear out of sync if they match
 *
 * \param  thingName         The thing to which the property belongs to
 *
 * \param  json              json returned in callback
 *
 */
void _AWS_ClearOutOfSyncFlag(
        const char* thingName,
        const char* json)
{
    jsmn_parser shadowJsonParser;
    jsmn_init(&shadowJsonParser);

    // We won't need as large a structure as we are only parsing a set of deltas
    jsmntok_t jsonTokenStruct[MAX_JSON_TOKEN_EXPECTED/2];

    int32_t tokenCount = jsmn_parse(&shadowJsonParser, json,
            strlen(json), jsonTokenStruct, sizeof(jsonTokenStruct) / sizeof(jsonTokenStruct[0]));

    if (tokenCount < 0)
    {
        LOG_Warning("Failed to parse JSON: %d", tokenCount);
        return;
    }

    uint32_t tokenIndex;

    // Find the "state" key
    if (!_AWS_IsJsonKeyMatching(json, jsonTokenStruct, tokenCount, "state",
            &tokenIndex))
    {
        LOG_Error("Did not find \"state\" key");
        return;
    }

    if ((tokenIndex+1) >= MAX_JSON_TOKEN_EXPECTED)
    {
        LOG_Error("We will walk off the end of the buffer while parsing JSON for state?");
        return; // Log error and bail, there is nothing else we can do
    }

    // Read the "state" JSON object
    int stateEnd = jsonTokenStruct[tokenIndex + 1].end;   // Next index is the JSMN_OBJECT
    int stateStart = tokenIndex + 2;        // Start of the first key-value pair

    // Now we look for reported or desired
    if (_AWS_IsJsonKeyMatching(json, jsonTokenStruct, tokenCount,
            "desired", &tokenIndex))
    {
        LOG_Trace("Not interested in desired changes");
        return;
    }

    // Now we look for reported or desired
    if (!_AWS_IsJsonKeyMatching(json, jsonTokenStruct, tokenCount,
            "reported", &tokenIndex))
    {
        LOG_Error("Did not find \"reported\" key");
        return;
    }

    if ((tokenIndex+1) >= MAX_JSON_TOKEN_EXPECTED)
    {
        LOG_Error("We will walk off the end of the buffer while parsing JSON for reported?");
        return; // Log error and bail, there is nothing else we can do
    }
    // Read the "reported" JSON object
    stateEnd = jsonTokenStruct[tokenIndex + 1].end;   // Next index is the JSMN_OBJECT
    stateStart = tokenIndex + 2;            // Start of the first key-value pair

    unsigned int pairIndex = stateStart;   // Start of the first key-value pair
    while (jsonTokenStruct[pairIndex].start <= stateEnd
            && pairIndex < tokenCount)
    {
        // Parse the key
        char keyName[LSD_PROPERTY_NAME_BUFFER_SIZE];
        EnsoErrorCode_e result = _AWS_ParseStringValue(keyName,
                LSD_PROPERTY_NAME_BUFFER_SIZE, json,
                &jsonTokenStruct[pairIndex]);
        if (eecNoError != result)
        {
            LOG_Error("parseStringValue for %s failed %s", keyName,
                    LSD_EnsoErrorCode_eToString(result));
            break;   // Early exit
        }

        // Check the type of the key in case we have nested properties
        if (JSMN_OBJECT == jsonTokenStruct[pairIndex + 1].type)
        {
            int nestedStart = pairIndex + 2;
            int nestedEnd = jsonTokenStruct[pairIndex + 1].end;
            pairIndex = _AWS_CheckKeyValuePairs(thingName, keyName,
                    json, nestedStart, nestedEnd, tokenCount, jsonTokenStruct);
        }
        else if ((JSMN_PRIMITIVE == jsonTokenStruct[pairIndex + 1].type)
                   || (JSMN_STRING == jsonTokenStruct[pairIndex + 1].type))
        {
            _AWS_VerifyPropertyValue(thingName, keyName, json,
                    jsonTokenStruct[pairIndex + 1]);

            // Next set of key-value pair
            pairIndex += 2;
        }
        else
        {
            LOG_Error("Unexpected json type %d",
                    jsonTokenStruct[pairIndex + 1].type);
            break;   // Early exit
        }
    }   // end of while
}

/*
 * \brief Parse a JSON key-value pair to update a property value.
 *
 * \param  thingName         The thing to which the property belongs to
 *
 * \param  propParentName    The property parent name (for nested properties only)
 *
 * \param  buffer            Json string to parse
 *
 * \param  indexStart        The starting index in the JSON token structure
 *
 * \param  positionEnd       The end position in the JSON data string
 *
 * \param  tokenCount        The number of parsed tokens
 *
 * \param  jsonTokenStruct   The structure describing the json tokens
 *
 * \return  int              The next index in the JSON token structure
 *
 */
static int _AWS_CheckKeyValuePairs(
        const char * thingName,
        const char * propParentName,
        const char * json,
        const int indexStart,
        const int positionEnd,
        const int tokenCount,
        jsmntok_t* jsonTokenStruct)
{
    unsigned int pairIndex = indexStart;   // Start of the first key-value pair
    while (jsonTokenStruct[pairIndex].start <= positionEnd
            && pairIndex < tokenCount)
    {
        // Parse the key
        char keyName[LSD_PROPERTY_NAME_BUFFER_SIZE];
        EnsoErrorCode_e result = _AWS_ParseStringValue(keyName,
        LSD_PROPERTY_NAME_BUFFER_SIZE, json, &jsonTokenStruct[pairIndex]);
        if (eecNoError != result)
        {
            LOG_Error("parseStringValue for %s failed %s", keyName,
                    LSD_EnsoErrorCode_eToString(result));
            // If the JSON can't be parsed then we have
            // bigger problems. Log the error and bail
            break;
        }

        // Parse the value
        char propName[LSD_PROPERTY_NAME_BUFFER_SIZE];
        snprintf(propName, LSD_PROPERTY_NAME_BUFFER_SIZE, "%s_%s",
                propParentName, keyName);

        _AWS_VerifyPropertyValue(thingName, propName, json,
                jsonTokenStruct[pairIndex + 1]);

        // Next set of key-value pair
        pairIndex += 2;
    }

    return pairIndex;
}


/*
 * \brief Parse a JSON value and check that it matches the property
 *
 * \param  thingName         The thing to which the property belongs to
 *
 * \param  propName          The property name
 *
 * \param  json              The json string
 *
 * \param  token             Which json token are we considering
 *
 * \return  Error code       eecNoError if all is well, something else otherwise
 */
EnsoErrorCode_e _AWS_VerifyPropertyValue(
        const char * thingName,
        const char * propName,
        const char * json,
        jsmntok_t token)
{
    assert(thingName);
    assert(propName);

    // First we need to find the value type of the property.
    // So we find the property in the Local Shadow and we look for the value type
    EnsoDeviceId_t theDevice;
    EnsoErrorCode_e retVal;

    retVal = LSD_GetThingFromNameString(thingName, &theDevice);

    if (eecNoError != retVal)
    {
        LOG_Error("LSD_GetThingFromNameString failed %d", retVal);
        return retVal;   // Should never happen
    }

    // Find property
    EnsoProperty_t* property = LSD_GetPropertyByCloudName(&theDevice, propName);
    if (!property)
    {
        LOG_Error("LSD_GetPropertyByCloudName failed, device %s prop %s",
                thingName, propName);
        return eecPropertyNotFound;   // Should never happen
    }

    retVal = eecNoError;   // Override as necessary

    int length = token.end - token.start;

    if (property->type.valueType == evString)
    {
        // Compare string, up to length chars
        if (strncmp(property->reportedValue.stringValue, &json[token.start],
                length) != 0)
        {
            LOG_Error("Callback for %s with %s did not match %s", propName,
                    property->reportedValue.stringValue, &json[token.start]);
            retVal = eecInternalError;
        }
    }
    else if (property->type.valueType == evBlobHandle)
    {
        // Firstly catch the empty string
        if (length == 0)
        {
            // Blob is either null or not null and zero length
            if (property->reportedValue.memoryHandle == NULL)
            {
                retVal = eecNoError;
            }
            else
            {
                if (((char*)property->reportedValue.memoryHandle)[0] == 0)
                {
                    // String is empty, this is ok
                    retVal = eecNoError;
                }
                else
                {
                    LOG_Error("Empty string from cloud does not match shadow?");
                    retVal = eecInternalError;
                }
            }
        }
        else
        {
            // Compare strings, up to length chars
            char str[length+1];
            retVal = _AWS_ParseStringValue(str, length+1, json, &token);

            if (retVal == eecNoError)
            {
                if (strncmp(property->reportedValue.memoryHandle, str, length) != 0)
                {
                    LOG_Error("Callback for %s with %s did not match %s", propName,
                            property->reportedValue.memoryHandle, str);
                    retVal = eecInternalError;
                }
            }
            else
            {
                LOG_Error("Failed to read string in json, %s?", LSD_EnsoErrorCode_eToString(retVal));
            }
        }
    }
    else
    {
        EnsoPropertyValue_u jsonValue;

        // Now we can parse the json token based on the value type
        IoT_Error_t rc = _AWS_ParsePrimitive(property->type.valueType,
                &jsonValue, json, token);

        if (SUCCESS != rc)
        {
            LOG_Error("AWS_ParsePrimitive for %s failed %s", propName,
                    _AWS_Strerror(rc));
            retVal = eecInternalError;
        }
        else
        {
            switch (property->type.valueType)
            {
                case evInt32:
                    if (jsonValue.int32Value != property->reportedValue.int32Value)
                    {
                        LOG_Info("Callback for %s with %i did not match %i",
                                propName, property->reportedValue.int32Value,
                                jsonValue.int32Value);
                        retVal = eecInternalError;
                    }
                    break;

                case evUnsignedInt32:
                    if (jsonValue.uint32Value
                            != property->reportedValue.uint32Value)
                    {
                        LOG_Info("Callback for %s with %u did not match %u",
                                propName, property->reportedValue.uint32Value,
                                jsonValue.uint32Value);
                        retVal = eecInternalError;
                    }
                    break;

                case evFloat32:
                    if (jsonValue.float32Value
                            != property->reportedValue.float32Value)
                    {
                        LOG_Info("Callback for %s with %f did not match %f",
                                propName, property->reportedValue.float32Value,
                                jsonValue.float32Value);
                        retVal = eecInternalError;
                    }
                    break;

                case evBoolean:
                    if (jsonValue.booleanValue
                            != property->reportedValue.booleanValue)
                    {
                        LOG_Info("Callback for %s with %s did not match %s",
                                propName,
                                property->reportedValue.booleanValue ? "true" : "false",
                                jsonValue.booleanValue ? "true" : "false" );
                        retVal = eecInternalError;
                    }
                    break;

                case evTimestamp:
                    if (jsonValue.timestamp.seconds
                            != property->reportedValue.timestamp.seconds)
                    {
                        LOG_Info("Callback for %s with %i did not match %i",
                                propName, property->reportedValue.timestamp.seconds,
                                jsonValue.timestamp.seconds);
                        retVal = eecInternalError;
                    }
                    break;

                default:
                    LOG_Error("Unsupported property type for %s", propName);
                    retVal = eecInternalError;
                    break;
            }
        }
    }

    static int notSyncingCounter = 0;

    if (retVal == eecNoError)
    {
        // We are in sync, update the property
        property->type.reportedOutOfSync = false;

        if (property->type.persistent)
        {
            // Want storage manager to record we are now in sync
            EnsoPropertyDelta_t delta;
            delta.agentSidePropertyID = property->agentSidePropertyID;
            delta.propertyValue = property->reportedValue;
            retVal = ECOM_SendUpdateToSubscriber(STORAGE_HANDLER, theDevice, REPORTED_GROUP, 1, &delta);
            if (retVal < 0)
            {
                LOG_Error("Failed to send reported for %s to storage handler", property->cloudName);
            }
        }

        // We are syncing can have a short recovery time now
        _commsDelayMs = aws_out_of_sync_short_shadow_recovery_interval_ms;

        notSyncingCounter = 0;
    }
    else
    {
        // We can have some false positives at start up so only start printing
        // errors when it happens repeatedly
        if (++notSyncingCounter > 1)
        {
            LOG_Error("Not setting in sync for %s", property->cloudName);
        }
        else
        {
            LOG_Info("Not setting in sync for %s", property->cloudName);
        }
    }

    return retVal;
}



/*
 * \brief  Generate last will and testament message
 *
 * \return  Error code    eecNoError or error
 */
EnsoErrorCode_e _AWS_GenerateLastWillAndTestament(void)
{
    snprintf(lastWillMessage, AWS_MAX_LENGTH_OF_LAST_WILL_MESSAGE,
            "{\"state\":{\"reported\":{\"%s\":0}}}",
            PROP_ONLINE_CLOUD_NAME);
    _connectParams.will.pMessage = lastWillMessage;
    _connectParams.will.msgLen = (uint16_t) strlen(_connectParams.will.pMessage);

    // Setup the Last Will and Testament topic for just Channel 0
    snprintf(lastWillTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES, "device/%s/offline", _gatewayThingName);
    _connectParams.will.pTopicName = lastWillTopic;
    _connectParams.will.topicNameLen = (uint16_t) strlen(_connectParams.will.pTopicName);

    return eecNoError;
}



/**
 * \brief   Send message to delete worker to start it processing delete queue
 *
 * \param   unused - handle needed when called from timer, but not used
 */
static void _AWS_KickDeleteWorker(void* unused)
{
    OSAL_SendMessage(_deleteMessageQueue, "0", 1, MessagePriority_high);
}



/**
 * \brief   Consumer thread - receives messages on mq
 *
 * \param   mq Message queue on which to listen.
 */
static void _AWS_ObjectDeleteWorker(MessageQueue_t mq)
{
    // For ever loop
    for ( ; ; )
    {
        char buffer[ECOM_MAX_MESSAGE_SIZE];
        MessagePriority_e priority;

        int size = OSAL_ReceiveMessage(mq, buffer, sizeof(buffer), &priority);
        if (size < 1)
        {
            LOG_Error("OSAL_ReceiveMessage error %d", size);
            continue;
        }

        if (!_awsCommsChannels[0].bConnected)
        {
            // If we are not connected then no point in trying, just try again later
            _AWS_SetTimerForDeleteWorker();
        }
        else
        {
            EnsoDeviceId_t deviceToDelete;

            if(AWS_GetHeadOfDeleteQueue(&deviceToDelete))
            {
                // Get the device's name
                char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];
                int bufferUsed;

                EnsoErrorCode_e retVal = LSD_GetThingName(thingName, sizeof(thingName), &bufferUsed, deviceToDelete, _gatewayId);
                if (eecNoError != retVal)
                {
                    LOG_Error("LSD_GetThingName failed for device %d", retVal);
                }
                else
                {
                    char parentName[ENSO_OBJECT_NAME_BUFFER_SIZE];

                    LOG_Info("Device to delete : %s", thingName);

                    // Get the gateway's name
                    retVal = LSD_GetThingName(parentName, sizeof(parentName), &bufferUsed, _gatewayId, _gatewayId);
                    if (eecNoError != retVal)
                    {
                        LOG_Error("LSD_GetThingName failed for gateway %d", retVal);
                    }
                    else
                    {
                        // Trying to delete from cloud, publish to lambda listening
                        snprintf(devDelTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES,
                                "device/%s/cancel", parentName);

                        IoT_Publish_Message_Params params;
                        memset(&params, 0, sizeof(params));
                        params.qos = QOS0;

                        char cPayload[100];
                        snprintf(cPayload, sizeof cPayload,  "{\"deviceId\":\"%s\"}", thingName);
                        params.payload = (void *) cPayload;
                        params.payloadLen = strlen(cPayload);

                        OSAL_LockMutex(&_commsMutex);
                        // Note that the following call is blocking but will return immediately
                        // if mqtt client is not connected
                        IoT_Error_t rc = aws_iot_mqtt_publish(
                            &_awsCommsChannels[0].mqttClient,  /* Publishes are always in connection 0 */
                            devDelTopic,
                            (uint16_t) strlen(devDelTopic),
                            &params);
                        OSAL_UnLockMutex(&_commsMutex);

                        if (SUCCESS != rc)
                        {
                            // Failed to delete device?
                            LOG_Error("Delete failed in aws_iot_mqtt_publish failed %d", rc);
                        }
                    }
                }

                // Set timer to look again in a short time whether it succeeded
                _AWS_SetTimerForDeleteWorker();
            }
        }
    }
}



/**
 * \name   _AWS_SetTimerForDeleteWorker
 * \brief  Start timer to call periodic to process messages
 */
static void _AWS_SetTimerForDeleteWorker(void)
{
    OSAL_LockMutex(&_timerMutex);

    if (_deleteTimer != NULL)
    {
        OSAL_DestroyTimer(_deleteTimer);
    }
    // Set timer longer than timeout for comms
    _deleteTimer = OSAL_NewTimer(_AWS_KickDeleteWorker, 30000, false, NULL);

    OSAL_UnLockMutex(&_timerMutex);
}



/**
 * \name   _AWS_AddToDeleteQueue
 * \brief  Start timer to call periodic to process messages
 * \param  device  device to delete
 */
static void _AWS_AddToDeleteQueue(EnsoDeviceId_t device) {
    _deleteQueue_t* head = malloc(sizeof(_deleteQueue_t));
    if(head == NULL)
    {
        LOG_Error("Failed to malloc space in deletion queue");
        return;
    }

    OSAL_LockMutex(&_deleteMutex);
    head->deviceToDelete = device;
    head->next = _deleteQueue;
    _deleteQueue = head;
    OSAL_UnLockMutex(&_deleteMutex);
}


/**
 * \name   _AWS_PopDeleteQueue
 * \brief  Remove device from our delete queue
 * \param  device  device to delete
 */
static void _AWS_RemoveFromDeleteQueue(char* deviceName) {
    bool found = false;

    // Search for the mac address in our list
    OSAL_LockMutex(&_deleteMutex);
    _deleteQueue_t* current = _deleteQueue;
    _deleteQueue_t** previous = &_deleteQueue;
    while(!found && current)
    {
        char thingName[ENSO_OBJECT_NAME_BUFFER_SIZE];
        int bufferUsed;

        EnsoErrorCode_e retVal = LSD_GetThingName(thingName, sizeof(thingName), &bufferUsed,
                current->deviceToDelete, _gatewayId);

        if (eecNoError == retVal)
        {
            if (strncmp(deviceName, thingName, ENSO_OBJECT_NAME_BUFFER_SIZE) == 0)
            {
                found = true;
                *previous = current->next;
                free(current);
                break;
            }
        }
        else
        {
            LOG_Error("LSD_GetThingName failed for device %d", retVal);
        }

        previous = &current;
        current = current->next;
    }
    OSAL_UnLockMutex(&_deleteMutex);

    if (!found)
    {
        LOG_Error("We did not find deleted thing (%s) in our list?", deviceName);
    }
}



/**
 * \name   AWS_GetHeadOfDeleteQueue
 * \brief  Copy head of queue out so that we can delete the device it refers to
 * \param  device  device to delete
 * \return true if we have a device to delete
 */
bool AWS_GetHeadOfDeleteQueue(EnsoDeviceId_t* device) {
    bool found = false;

    OSAL_LockMutex(&_deleteMutex);
    if (_deleteQueue != NULL)
    {
        found = true;
        *device = _deleteQueue->deviceToDelete;
    }
    OSAL_UnLockMutex(&_deleteMutex);

    return found;
}



/**
 * \name   _AWS_GatewayAnnounceAndSubscriptions
 * \brief  Do all cloud announce and subscriptions for the gateway
 * \param  parentName   Gateway name
 * \param  owner        owner
 * \return true if it all succeeds
 */
static bool _AWS_GatewayAnnounceAndSubscriptions(char* parentName, EnsoObject_t* owner)
{
    // Gateway always uses Channel 0 for Accept/Reject Subscriptions and Topic Publishes
    AWS_CLD_CommsChannel_t* channel0 = &_awsCommsChannels[0];

    // Announcing the gateway.
    // Subscribe to the device-announce-accept topic
    snprintf(devAnnAcceptTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES,
          "device/%s/announce/accept", parentName);

    OSAL_LockMutex(&_commsMutex);
    // Note that the following call is blocking
    IoT_Error_t rc = aws_iot_mqtt_subscribe(
        &channel0->mqttClient, /* Announce Accept subscription always in connection 0 */
        devAnnAcceptTopic,
        (uint16_t) strlen(devAnnAcceptTopic),
        QOS0,
        _AWS_AnnounceAccept,
        NULL);
    OSAL_UnLockMutex(&_commsMutex);

    if (SUCCESS != rc)
    {
        LOG_Error("aws_iot_mqtt_subscribe failed 1 %d", rc);
        _commsDelayMs = aws_out_of_sync_initial_shadow_recovery_interval_ms;

        /*
         * If gateway does not subscribe, must retry again.
         * Clear flags to retry later in _AWS_IOT_SendOutOfSyncShadows() function
         */
        owner->announceInProgress = false;
        owner->announceAccepted = false;

        // Announce retries are handled together with out of sync function
        return false;
    }
    else
    {
        LOG_Info("Subscribed to %s", devAnnAcceptTopic);

        /* Add subscription to channel0  */
        channel0->subscriptionCount++;
    }

    // Subscribe to the device-announce-reject topic
    snprintf(devAnnRejectTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES,
             "device/%s/announce/reject", parentName);

    OSAL_LockMutex(&_commsMutex);
    // Note that the following call is blocking
    rc = aws_iot_mqtt_subscribe(
        &channel0->mqttClient, /* Announce Accept subscription always in connection 0 */
        devAnnRejectTopic,
        (uint16_t) strlen(devAnnRejectTopic),
        QOS0,
        _AWS_AnnounceReject,
        NULL);
    OSAL_UnLockMutex(&_commsMutex);

    if (SUCCESS != rc)
    {
        LOG_Error("aws_iot_mqtt_subscribe failed 2 %d", rc);
        _commsDelayMs = aws_out_of_sync_initial_shadow_recovery_interval_ms;

        /*
         * If gateway does not subscribe, must retry again.
         * Clear flags to retry later in _AWS_IOT_SendOutOfSyncShadows() function
         */
        owner->announceInProgress = false;
        owner->announceAccepted = false;

        // Announce retries are handled together with out of sync function
        return false;
    }
    else
    {
        LOG_Info("Subscribed to %s", devAnnRejectTopic);

        /* Add subscription to channel0  */
        channel0->subscriptionCount++;
    }

    snprintf(devDeleteAcceptTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES,
                 "$aws/things/%s/shadow/delete/accepted", parentName);

    OSAL_LockMutex(&_commsMutex);
    // Note that the following call is blocking
    rc = aws_iot_mqtt_subscribe(
        &channel0->mqttClient, /* Announce Accept subscription always in connection 0 */
        devDeleteAcceptTopic,
        (uint16_t) strlen(devDeleteAcceptTopic),
        QOS1,
        _AWS_GWDeleteAccept,
        parentName);
    OSAL_UnLockMutex(&_commsMutex);

    if (SUCCESS != rc)
    {
        LOG_Error("aws_iot_mqtt_subscribe failed for Delete Accept %s", _AWS_Strerror(rc));
        _commsDelayMs = aws_out_of_sync_initial_shadow_recovery_interval_ms;

        /*
         * If gateway does not subscribe, must retry again.
         * Clear flags to retry later in _AWS_IOT_SendOutOfSyncShadows() function
         */
        owner->announceInProgress = false;
        owner->announceAccepted = false;

        // Announce retries are handled together with out of sync function
        return false;
    }
    else
    {
        LOG_Info("Subscribed to %s", devDeleteAcceptTopic);

        /* Add subscription to channel0  */
        channel0->subscriptionCount++;
    }

    // Subscribe to cancels
    snprintf(devCancelTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES, "device/%s/cancel/accept", parentName);

    LOG_Info("Trying to subscribe to %s", devCancelTopic);

    OSAL_LockMutex(&_commsMutex);
    // Note that the following call is blocking
    rc = aws_iot_mqtt_subscribe(
        &channel0->mqttClient,
        devCancelTopic,
        (uint16_t) strlen(devCancelTopic),
        QOS1,
        _AWS_CancelAccept,
        parentName);
    OSAL_UnLockMutex(&_commsMutex);

    if (SUCCESS != rc)
    {
        LOG_Error("aws_iot_mqtt_subscribe failed for Cancel Accept: %s", _AWS_Strerror(rc));
        _commsDelayMs = aws_out_of_sync_initial_shadow_recovery_interval_ms;

        owner->announceInProgress = false;
        owner->announceAccepted = false;

        // Announce retries are handled together with out of sync function
        return false;
    }
    else
    {
        LOG_Info("Subscribed to %s", devCancelTopic);

        /* Add subscription to channel0  */
        channel0->subscriptionCount++;
    }

    return true;
}

/**
 * \name   _AWS_AnnounceNewThing
 * \brief  Do cloud announce and all things including GW
 * \param  owner Owner Objects
 * \param  deviceID Thing Device ID
 * \param  parentName Parent Thing Name
 * \param  thingName to be announced Thing Name
 * \param  isGateway Is thing GW
 * \param  thingType Thing Type
 * \param  connectionID AWS Connection ID for thing
 *
 * \return none
 */
static void _AWS_AnnounceNewThing(EnsoObject_t* owner, EnsoDeviceId_t* deviceID, char* parentName, char* thingName, bool isGateway, uint32_t thingType, int32_t connectionID)
{
    // Publish to the device-announce topic
    snprintf(devAnnTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES, "device/%s/announce", parentName);

    LOG_Info("devAnnTopic %s", devAnnTopic);

    IoT_Publish_Message_Params params;
    memset(&params, 0, sizeof(params));
    params.qos = QOS0;
    //params.isRetained = false;
    //params.isDup = false;
    //params.id = 0;
    char cPayload[150];

    if (isGateway)
    {
        /* GW needs to announce device ID and type : See G2-410 */
        ////////snprintf(cPayload, sizeof cPayload, "{ \"deviceId\": \"%s\", \"type\": %u }", parentName, thingType);	//////// [RE:format]
        snprintf(cPayload, sizeof cPayload, "{ \"deviceId\": \"%s\", \"type\": %lu }", parentName, thingType);
    }
    else
    {
        /* Sensor needs to announce device ID, parent ID and type : See G2-410 */
        ////////snprintf(cPayload, sizeof cPayload,  "{ \"deviceId\": \"%s\", \"parent\": \"%s\", \"type\": %u }", thingName, parentName, thingType);	//////// [RE:format]
        snprintf(cPayload, sizeof cPayload,  "{ \"deviceId\": \"%s\", \"parent\": \"%s\", \"type\": %lu }", thingName, parentName, thingType);
    }

    params.payload = (void *) cPayload;
    params.payloadLen = strlen(cPayload);

    EnsoPropertyValue_u connVal;
    connVal.int32Value = connectionID;

    // Set the connection ID property
    EnsoErrorCode_e retVal = LSD_SetPropertyValueByAgentSideId(
                COMMS_HANDLER,
                deviceID,
                REPORTED_GROUP,
                PROP_CONNECTION_ID,
                connVal);

    if (eecNoError != retVal)
    {
        LOG_Info("LSD_SetPropertyValueByAgentSideId err %s", LSD_EnsoErrorCode_eToString(retVal));
    }

    /* Increase the subscription count of channel */
    _awsCommsChannels[connectionID].subscriptionCount++;

    OSAL_LockMutex(&_commsMutex);
    AWS_CLD_CommsChannel_t* channel0 = &_awsCommsChannels[0];

    // Note that the following call is blocking but
    // will return immediately if mqtt client is not
    // connected
    IoT_Error_t rc = aws_iot_mqtt_publish(
            &channel0->mqttClient, /* Publishes are always in connection 0 */
            devAnnTopic,
            (uint16_t) strlen(devAnnTopic),
            &params);
    OSAL_UnLockMutex(&_commsMutex);


    if (owner)
    {
        /* Success or fail, clear flag to exit from InProgress State */
        owner->announceInProgress = false;
    }

    if (SUCCESS != rc)
    {
        // Failed, don't set announced flag, we will try again on reconnect
        LOG_Error("aws_iot_mqtt_publish failed %d", rc);
    }
    else
    {
        LOG_Info("Published to %s - %s", devAnnTopic, params.payload);

        /* To fix race condition, a quick fix. Will be fixed later */
        if (owner)
        {
            owner->announceAccepted = true;
        }
    }
}
#endif
