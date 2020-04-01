/*!****************************************************************************
 *
 * \file GW_Handler.c
 *
 * \author Murat Cakmak
 *
 * \brief Reset Device Handler
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "SYS_Gateway.h"
#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "LSD_Api.h"
#include "LOG_Api.h"
#include "HAL.h"
#include "APP_Types.h"

#include <string.h>

#define RESET_WAIT_TIME_IN_MS           (10000)

/*
 * Demo Properties
 */
#define PROP_DEMO_LTMOUT_ID                     (PROP_GROUP_GATEWAY | 0x0030)
#define PROP_DEMO_LTRGRD_ID                     (PROP_GROUP_GATEWAY | 0x0031)
#define PROP_DEMO_UNKN_ID                       (PROP_GROUP_GATEWAY | 0x0032)
#define PROP_DEMO_TESTMODE_ID                   (PROP_GROUP_GATEWAY | 0x0033)
#define PROP_DEMO_TESTRES_ID                    (PROP_GROUP_GATEWAY | 0x0034)

static EnsoDeviceId_t _gatewayDeviceId = { 0 };

/**
 * \brief Updates local shadow for reset and resets Gateway
 *
 * \param delta shadow delta
 *
 * \return Success or Error code
 *
 */
static EnsoErrorCode_e _ResetDevice(EnsoPropertyDelta_t* delta)
{
    /* Delta reports reset time (timestamp) */
    uint32_t timestamp = delta->propertyValue.uint32Value;

    /* Get "reset" Property */
    EnsoProperty_t* property = LSD_GetPropertyByAgentSideId(&_gatewayDeviceId, PROP_GW_RESET_ID);

    if (property == NULL)
    {
        LOG_Error("Invalid Property!");
        return eecPropertyNotFound;
    }

    /* Allow to change if reset value is bigger than previous reset time */
    if (timestamp > property->reportedValue.uint32Value)
    {
        /* Set reported value */
        EnsoErrorCode_e ret = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                &_gatewayDeviceId,
                                                                REPORTED_GROUP,
                                                                PROP_GW_RESET_ID,
                                                                delta->propertyValue);

        if (ret != eecNoError)
        {
            LOG_Error("Failed to set Gateway Reset Property")
            return ret;
        }

        uint32_t retryCount = RESET_WAIT_TIME_IN_MS / 1000;

        do
        {
            /* Delta is reported before reset so no need to wait more */
            if (property->type.reportedOutOfSync == false)
            {
                break;
            }

            OSAL_sleep_ms(1000);

        } while (retryCount-- > 0);

        /* Reboot device */
        HAL_Reboot();
    }

    return eecNoError;
}

/**
 * \brief Send GW Status to subscriber
 *
 * \param subscriberID Subscriber ID
 * \param gwStatus New Gateway Status
 *
 * \return none
 *
 */
EnsoErrorCode_e ECOM_SendGWStatusToSubscriber(HandlerId_e subscriberID, EnsoGateWayStatus_e gwStatus)
{
    // Prepare the message
    ECOM_GatewayStatus gwStatusMessage = (ECOM_GatewayStatus) {
            .messageId = ECOM_GATEWAY_STATUS,
            .status = gwStatus
    };

    EnsoErrorCode_e retVal = eecNoError;

    // Get the message queue for this thing
    MessageQueue_t destQueue = ECOM_GetMessageQueue(subscriberID);

    if (destQueue)
    {
        if (OSAL_SendMessage(destQueue, &gwStatusMessage, sizeof(ECOM_GatewayStatus), MessagePriority_medium) < 0)
        {
            retVal = eecInternalError;
            LOG_Error("OSAL_SendMessage failed %s", strerror(errno));
        }
    }
    else
    {
        LOG_Trace("No handler on destination queue, ok at startup though");
    }

    return retVal;
}

EnsoErrorCode_e WiSafe_DALRegisterDevice(uint32_t id, uint32_t deviceType)
{
    EnsoDeviceId_t ensoId;
    ensoId.deviceAddress = id;
    ensoId.childDeviceId = 0;
    ensoId.technology = WISAFE_TECHNOLOGY;
    ensoId.isChild = true;

    (void)deviceType;

    EnsoObject_t* newObject = LSD_CreateEnsoObject(ensoId);
    if (newObject != NULL)
    {
        // Create the owner (persistent private) property.
        EnsoPropertyValue_u handlerValue[PROPERTY_GROUP_MAX];
        handlerValue[DESIRED_GROUP].uint32Value = WISAFE_DEVICE_HANDLER;
        handlerValue[REPORTED_GROUP].uint32Value = WISAFE_DEVICE_HANDLER;

        EnsoErrorCode_e result = LSD_CreateProperty(
                newObject,
                PROP_OWNER_ID,
                PROP_OWNER_INTERNAL_NAME,
                evUnsignedInt32,
                PROPERTY_PRIVATE,
                false,
                true,
                handlerValue);
        if (result < 0)
        {
            LOG_Error("Failed to create owner property %s", LSD_EnsoErrorCode_eToString(result));
        }

        // Create the device status private property.
        EnsoPropertyValue_u deviceStatusValue[PROPERTY_GROUP_MAX];
        deviceStatusValue[DESIRED_GROUP].uint32Value = THING_CREATED;
        deviceStatusValue[REPORTED_GROUP].uint32Value = THING_CREATED;

        result = LSD_CreateProperty(
                newObject,
                PROP_DEVICE_STATUS_ID,
                PROP_DEVICE_STATUS_CLOUD_NAME,
                evUnsignedInt32,
                PROPERTY_PRIVATE,
                false,
                false,
                deviceStatusValue);
        if (result < 0)
        {
            LOG_Error("Failed to create device status property %s", LSD_EnsoErrorCode_eToString(result));
        }

        // Create the connection (private) property.
        EnsoPropertyValue_u connValue[PROPERTY_GROUP_MAX];
        connValue[DESIRED_GROUP].int32Value = -1;
        connValue[REPORTED_GROUP].int32Value = -1;

        result = LSD_CreateProperty(
                newObject,
                PROP_CONNECTION_ID,
                PROP_CONNECTION_ID_CLOUD_NAME,
                evInt32,
                PROPERTY_PRIVATE,
                false,
                false,
                connValue);
        if (result < 0)
        {
            LOG_Error("Failed to create connection property %s", LSD_EnsoErrorCode_eToString(result));
        }

        #define DAL_PROPERTY_DEVICE_TYPE_WISAFE_MASK 0x00010000
        // Create the connection (private) property.
        EnsoPropertyValue_u typeValue[PROPERTY_GROUP_MAX];
        typeValue[DESIRED_GROUP].uint32Value = (id | DAL_PROPERTY_DEVICE_TYPE_WISAFE_MASK) ;
        typeValue[REPORTED_GROUP].uint32Value = (id | DAL_PROPERTY_DEVICE_TYPE_WISAFE_MASK) ;

        result = LSD_CreateProperty(
                newObject,
                PROP_TYPE_ID,
                PROP_TYPE_CLOUD_NAME,
                evUnsignedInt32,
                PROPERTY_PUBLIC,
                false,
                false,
                typeValue);

        if (result < 0)
        {
            LOG_Error("Failed to create connection property %s", LSD_EnsoErrorCode_eToString(result));
        }

        #define DAL_PROP_ID(x)                           (PROP_GROUP_WISAFE | x)
        #define DAL_PROPERTY_DEVICE_LAST_SEQUENCE_IDX     DAL_PROP_ID(14)
        //const propertyName_t DAL_PROPERTY_DEVICE_LAST_SEQUENCE        = (propertyName_t)"ltseq";


        EnsoPropertyValue_u ltseqValue[PROPERTY_GROUP_MAX];
        ltseqValue[DESIRED_GROUP].uint32Value = 3 ;
        ltseqValue[REPORTED_GROUP].uint32Value = 3 ;

        result = LSD_CreateProperty(
                newObject,
                DAL_PROPERTY_DEVICE_LAST_SEQUENCE_IDX,
                "ltseq",
                evUnsignedInt32,
                PROPERTY_PUBLIC,
                false,
                false,
                ltseqValue);

        if (result < 0)
        {
            LOG_Error("Failed to create %d property %s", __LINE__);
        }


        //const char PROP_MANUFACTURER_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE]  = "mfr";

        EnsoPropertyValue_u mfrValue[PROPERTY_GROUP_MAX];
        strcpy(mfrValue[DESIRED_GROUP].stringValue, "Sprue");
        strcpy(mfrValue[REPORTED_GROUP].stringValue, "Sprue") ;

        result = LSD_CreateProperty(
                newObject,
                PROP_MANUFACTURER_ID,
                "mfr",
                evString,
                PROPERTY_PUBLIC,
                false,
                false,
                mfrValue);

        if (result < 0)
        {
            LOG_Error("Failed to create %d property %s", __LINE__);
        }

        //const propertyName_t DAL_PROPERTY_DEVICE_MODEL                = (propertyName_t)"mdVal";
        #define DAL_PROPERTY_DEVICE_MODEL_IDX             DAL_PROP_ID(5)
        EnsoPropertyValue_u mdVal[PROPERTY_GROUP_MAX];
        mdVal[DESIRED_GROUP].uint32Value = 1093 ;
        mdVal[REPORTED_GROUP].uint32Value = 1093 ;

        result = LSD_CreateProperty(
                newObject,
                DAL_PROPERTY_DEVICE_MODEL_IDX,
                "mdVal",
                evUnsignedInt32,
                PROPERTY_PUBLIC,
                false,
                false,
                mdVal);

        if (result < 0)
        {
            LOG_Error("Failed to create %d property %s", __LINE__);
        }

        EnsoPropertyValue_u onln[PROPERTY_GROUP_MAX];
        onln[DESIRED_GROUP].uint32Value = 1 ;
        onln[REPORTED_GROUP].uint32Value = 1 ;

        result = LSD_CreateProperty(
                newObject,
                PROP_ONLINE_ID,        "onln",
                evUnsignedInt32,
                PROPERTY_PUBLIC,
                false,
                false,
                onln);

        if (result < 0)
        {
            LOG_Error("Failed to create %d property %s", __LINE__);
        }


        //const char PROP_MODEL_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE]         = "model";
        #define PROP_MODEL_ID                           (PROP_GROUP_GATEWAY | 0x0003)
        EnsoPropertyValue_u model[PROPERTY_GROUP_MAX];
        strcpy(model[DESIRED_GROUP].stringValue, "ACU.0445") ;
        strcpy(model[REPORTED_GROUP].stringValue, "ACU.0445") ;

        result = LSD_CreateProperty(
                    newObject,
                    PROP_MODEL_ID,
                    "model",
                    evString,
                    PROPERTY_PUBLIC,
                    false,
                    false,
                    model);

        if (result < 0)
        {
            LOG_Error("Failed to create %d property %s", __LINE__);
        }

        result = LSD_RegisterEnsoObject(newObject);
        return result;
    }
    else
    {
       return eecEnsoObjectAlreadyRegistered;
    }
}

/**
 * \brief   Reset Handler - receives messages on mq
 *
 * \param   mq Message queue on which to listen.
 */
void GW_Handler(MessageQueue_t mq)
{
    static uint32_t newDeviceID = 1;

    for (;;)
    {
        char msg[ECOM_MAX_MESSAGE_SIZE];
        MessagePriority_e pri;
        int size = OSAL_ReceiveMessage(mq, msg, sizeof msg, &pri);

        uint8_t messageId = msg[0];
        ECOM_DeltaMessage_t * dm = (ECOM_DeltaMessage_t*)msg;

        if (size < 1)
        {
            LOG_Error("ReceiveMessage error %d", size);
            continue;
        }
        else if (messageId != ECOM_DELTA_MSG)
        {
            LOG_Error("Expected ECOM_DELTA_MSG(%d), got %d", ECOM_DELTA_MSG, messageId);
        }
        else if (size != sizeof (ECOM_DeltaMessage_t))
        {
            LOG_Error("Expected ECOM_DELTA_MSG size(%d), got %d", sizeof (ECOM_DeltaMessage_t), size);
        }
        else if (DESIRED_GROUP != dm->propertyGroup)
        {
            LOG_Error("Expected DESIRED_GROUP(%d), got %d", DESIRED_GROUP, dm->propertyGroup);
        }
        else if (GW_HANDLER != dm->destinationId)
        {
            LOG_Error("Expected Destination ID(%d), got %d", GW_HANDLER, dm->destinationId);
        }
        else
        {
            EnsoErrorCode_e retVal;

            for (int i = 0; i < dm->numProperties; i++)
            {
                EnsoPropertyDelta_t* delta = &dm->deltasBuffer[i];

                switch (delta->agentSidePropertyID)
                {
                    case PROP_GW_RESET_ID:
                        /* Reset Gateway */
                        _ResetDevice(delta);
                        break;
                    case PROP_GW_REGISTERED_ID:
                        {

                            /* Update reported property back */
                            retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                       &_gatewayDeviceId,
                                                                       REPORTED_GROUP,
                                                                       PROP_GW_REGISTERED_ID,
                                                                       delta->propertyValue);

                            if (retVal == eecNoError)
                            {
                                EnsoGateWayStatus_e gwStatus =
                                        delta->propertyValue.booleanValue ?
                                                GATEWAY_REGISTERED :
                                                GATEWAY_NOT_REGISTERED;

                                /* Commshandler needs also GW Status */
                                retVal = ECOM_SendGWStatusToSubscriber(COMMS_HANDLER, gwStatus);

                                if (retVal != eecNoError)
                                {
                                    LOG_Error("Failed to send update to subscriber");
                                }
                            }
                            else
                            {
                                LOG_Error("Failed to set property value");
                            }
                        }
                        break;
                    case PROP_ONLINE_SEQ_NO_ID:
                        {
                            LOG_Trace("Setting ONLNS = %s", delta->propertyValue.stringValue);
                            retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                       &_gatewayDeviceId,
                                                                       REPORTED_GROUP,
                                                                       PROP_ONLINE_SEQ_NO_ID,
                                                                       delta->propertyValue);

                            /*
                             * We need to set online property to 1 to avoid race
                             * condition between LTW
                             * See G2-2684
                             */
                            EnsoPropertyValue_u onlnVal = { 0 };
                            onlnVal.uint32Value = 1;
                            retVal =  LSD_SetPropertyValueByAgentSideId(COMMS_HANDLER,
                                                                        &_gatewayDeviceId,
                                                                        REPORTED_GROUP,
                                                                        PROP_ONLINE_ID,
                                                                        onlnVal);
                        }
                        break;
                    case PROP_STATE_ID:
                        {
                            GatewayState_e state = (GatewayState_e)delta->propertyValue.uint32Value;

                            switch (state)
                            {
                                case GatewayStateTestMode:
                                    {
                                        //LOG_Warning("Wait for Test ");
                                        OSAL_sleep_ms(5000);
                                        //LOG_Warning("Test is done!");

                                        EnsoPropertyValue_u tstResVal = { .booleanValue = true };
                                        retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                                   &_gatewayDeviceId,
                                                                                   REPORTED_GROUP,
                                                                                   PROP_DEMO_TESTRES_ID,
                                                                                   tstResVal);

                                        EnsoPropertyValue_u valueS;
                                        valueS.uint32Value = GatewayStateRunning;
                                        retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                                   &_gatewayDeviceId,
                                                                                   REPORTED_GROUP,
                                                                                   PROP_STATE_ID,
                                                                                   valueS);
                                    }
                                    break;
                                default:
                                    //LOG_Warning("\r\nUnhandled GW State %d \r\n", state);
                                    break;
                            }
                        }
                        break;
                    case PROP_DEMO_LTMOUT_ID:
                        /* Update reported property back */
                        retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                    &_gatewayDeviceId,
                                                                    REPORTED_GROUP,
                                                                    PROP_DEMO_LTMOUT_ID,
                                                                    delta->propertyValue);

                         EnsoPropertyValue_u valueS = { .uint32Value = GatewayStateLearning };
                         retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                    &_gatewayDeviceId,
                                                                    REPORTED_GROUP,
                                                                    PROP_STATE_ID,
                                                                    valueS);

                         EnsoPropertyValue_u unknVal;
                         unknVal.uint32Value = 1;
                         retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                    &_gatewayDeviceId,
                                                                    REPORTED_GROUP,
                                                                    PROP_DEMO_UNKN_ID,
                                                                    unknVal);

                         WiSafe_DALRegisterDevice(newDeviceID++, 0);

                         if (retVal != eecNoError)
                         {
                             LOG_Warning("Failed to WiSafe_DALRegisterDevice %d", retVal);
                         }

                         //LOG_Warning("Wait for device");
                         OSAL_sleep_ms(10000);
                         //LOG_Warning("Set unknown to 0");

                         unknVal.uint32Value = 0;
                         retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                    &_gatewayDeviceId,
                                                                    REPORTED_GROUP,
                                                                    PROP_DEMO_UNKN_ID,
                                                                    unknVal);


                         EnsoPropertyValue_u tstTrgVal;
                         tstTrgVal.uint32Value = OSAL_GetTimeInSecondsSinceEpoch();
                         retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                    &_gatewayDeviceId,
                                                                    REPORTED_GROUP,
                                                                    PROP_DEMO_TESTMODE_ID,
                                                                    tstTrgVal);


                         EnsoPropertyValue_u tstResVal = { .booleanValue = true };
                         retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                    &_gatewayDeviceId,
                                                                    REPORTED_GROUP,
                                                                    PROP_DEMO_TESTRES_ID,
                                                                    tstResVal);

                         valueS.uint32Value = GatewayStateRunning;
                         retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                    &_gatewayDeviceId,
                                                                    REPORTED_GROUP,
                                                                    PROP_STATE_ID,
                                                                    valueS);
                        break;
                    case PROP_DEMO_LTRGRD_ID:
                        /* Update reported property back */
                        retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                   &_gatewayDeviceId,
                                                                   REPORTED_GROUP,
                                                                   PROP_DEMO_LTRGRD_ID,
                                                                   delta->propertyValue);
                        break;
                    case PROP_DEMO_TESTMODE_ID:
                        /* Update reported property back */
                        retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                   &_gatewayDeviceId,
                                                                   REPORTED_GROUP,
                                                                   PROP_DEMO_TESTMODE_ID,
                                                                   delta->propertyValue);
                        break;
                    default:
                        LOG_Error("Invalid Property(%d)", delta->agentSidePropertyID)
                        break;
                }
            }
        }
    }
}

EnsoErrorCode_e GW_Initialise(void)
{
    /* Create a message queue and listener thread to get reset messages */
    const char * name = "/gwDH";
    LOG_Info("%s", name);
    SYS_GetDeviceId(&_gatewayDeviceId);
    MessageQueue_t mq = OSAL_NewMessageQueue(name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
    if (mq == NULL)
    {
        LOG_Error("OSAL_NewMessageQueue(\"%s\", %d, %d) failed", name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
        return eecInternalError;
    }
    Thread_t listeningThread = OSAL_NewThread(GW_Handler, mq);
    if (listeningThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for %s", name);
        return eecInternalError;
    }

    return ECOM_RegisterMessageQueue(GW_HANDLER, mq);
}
