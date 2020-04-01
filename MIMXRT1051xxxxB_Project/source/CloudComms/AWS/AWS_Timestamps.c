
/*!****************************************************************************
*
* \file AWS_Timestamps.c
*
* \author Gavin Dolling
*
* \brief Correct timestamps after we have got correct time
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include "AWS_Timestamps.h"
#include "ECOM_Api.h"
#include "LSD_Types.h"
#include "LSD_Api.h"
#include "LOG_Api.h"
#include "ECOM_Messages.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/

/******************************************************************************
 * Type Definitions
 *****************************************************************************/

/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

bool _timeIsValid;
uint32_t _timeAtStart;
uint32_t _timeAdjustment;
static Timer_t periodicCheckTimer = NULL;

static Thread_t periodicCheckTimeThread;

static Semaphore_t _periodicCheckTimeSemaphore;


/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static void _TimestampCorrectionQueueListener(MessageQueue_t mq);

static EnsoErrorCode_e _TimestampCheck(const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId, const PropertyGroup_e propertyGroup,
        const uint16_t numPropertiesChanged, const EnsoPropertyDelta_t* deltasBuffer);

static void _PeriodicCheckTimeCB(void* p);
static void _PeriodicCheckTime(void* p);


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/*
 * \brief  Timestamp initialisation code
 * \return Error code
 */

EnsoErrorCode_e AWS_TimestampInit()
{
    EnsoPropertyValue_u testTime = LSD_GetTimeNow();

    _timeIsValid = testTime.timestamp.isValid;
    _timeAtStart = testTime.timestamp.seconds;

    _timeAdjustment = 0;

    // Create check time thread
    if (OSAL_InitBinarySemaphore(&_periodicCheckTimeSemaphore) != 0)
    {
        LOG_Error("OSAL_InitBinarySemaphore() failed for AWS timestamps periodic check time semaphore");
        return eecInternalError;
    }
    periodicCheckTimeThread = OSAL_NewThread(_PeriodicCheckTime, NULL);
    if (periodicCheckTimeThread == NULL)
    {
        LOG_Error("OSAL_NewThread() failed for AWS timestamps periodic check time thread");
        return eecInternalError;
    }

    // If time is not valid start periodic checking
    if (!_timeIsValid)
    {
        LOG_Warning("Invalid Time, starting _PeriodicCheckTime");
        _PeriodicCheckTimeCB(NULL);
    }

    // Create message queue for timestamp handler
    const char * name = "/awsTS";

    MessageQueue_t messageQ = OSAL_NewMessageQueue(name, ECOM_MAX_MESSAGE_QUEUE_DEPTH,
        ECOM_MAX_MESSAGE_SIZE);
    if (messageQ == NULL)
    {
        LOG_Error("OSAL_NewMessageQueue(\"%s\", %d, %d) failed", name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
        return eecInternalError;
    }
    // Create listening thread
    Thread_t listeningThread = OSAL_NewThread(_TimestampCorrectionQueueListener, messageQ);
    if (listeningThread == NULL)
    {
        LOG_Error("OSAL_NewThread() failed for timestamp handler");
        return eecInternalError;
    }

    ECOM_RegisterMessageQueue(TIMESTAMP_HANDLER, messageQ);

    // Only used on implementations without messaging
    ECOM_RegisterOnUpdateFunction(TIMESTAMP_HANDLER, _TimestampCheck);

    return eecNoError;
}


/*
 * \brief Handler function for processing property change deltas
 * \return                   Whether we have valid time or not
 */
bool AWS_IsTimeValid()
{
    return _timeIsValid;
}


/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * \brief   Consumer thread - receives messages on mq
 * \param   mq Message queue on which to listen.
 */
static void _TimestampCorrectionQueueListener(MessageQueue_t mq)
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

        // Read the message id
        uint8_t messageId = buffer[0];
        switch (messageId)
        {
            case ECOM_DELTA_MSG:
                if (size != sizeof(ECOM_DeltaMessage_t))
                {
                    LOG_Error("Invalid DELTA message: size is %d, expected %d", size, sizeof(ECOM_DeltaMessage_t));
                }
                else
                {
                    ECOM_DeltaMessage_t* pDeltaMessage = (ECOM_DeltaMessage_t*)buffer;
                    _TimestampCheck(
                            pDeltaMessage->destinationId,
                            pDeltaMessage->deviceId,
                            pDeltaMessage->propertyGroup,
                            pDeltaMessage->numProperties,
                            pDeltaMessage->deltasBuffer);
                }
                break;

            case ECOM_LOCAL_SHADOW_STATUS:
                // This is sent when the filter finishes
                break;

            default:
                LOG_Error("Strange message sent to timestamp queue %d", messageId);
                break;
        }
    }
}



/**
 * \name   _TimestampCheck
 * \brief  Called to check timestamps of properties and update if necessary
 * \param  subscriberId      The subscriber i.e the destination handler
 * \param  publishedDeviceId The device that changed so is publishing the
 *                           update to its subscriber list.
 * \param  propertyGroup     Which group it effects
 * \param  numProperties     The number of properties changed
 * \param  deltasBuffer      Array of changes
 * \return EnsoErrorCode_e 0 = success else error code
 */
static EnsoErrorCode_e _TimestampCheck(const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId, const PropertyGroup_e propertyGroup,
        const uint16_t numPropertiesChanged, const EnsoPropertyDelta_t* deltasBuffer)
{
    EnsoErrorCode_e retVal = eecNoError;

    if (_timeAdjustment == 0)
    {
        LOG_Error("We should never be asked to do time delta when it is zero?");
    }

    if (propertyGroup == DESIRED_GROUP)
    {
        // We are not interested in changes to desired and shouldn't get these
        LOG_Error("DESIRED_GROUP update for timestamp handler %d : numProperties %d - ignoring",
                subscriberId, numPropertiesChanged);
    }
    else
    {
        for (int i=0; i<numPropertiesChanged; i++)
        {
            EnsoAgentSidePropertyId_t agentId = deltasBuffer[i].agentSidePropertyID;
            EnsoPropertyValue_u value = deltasBuffer->propertyValue;

            if (!value.timestamp.isValid)
            {
                value.timestamp.seconds += _timeAdjustment;
                value.timestamp.isValid = true;

                LSD_SetPropertyValueByAgentSideId(TIMESTAMP_HANDLER,
                        &publishedDeviceId, REPORTED_GROUP, agentId, value);
            }
        }
    }

    return retVal;
}



/**
 * \name   _PeriodicCheckTimeCB
 * \brief  Called once per second to test for time being valid
 * \param  p     not used
 */
static void _PeriodicCheckTimeCB(void* p)
{
    OSAL_GiveBinarySemaphore(&_periodicCheckTimeSemaphore);
}



/**
 * \name   _PeriodicCallBack
 * \brief  Called once per second to test for time being valid
 * \param  p     not used
 */
static void _PeriodicCheckTime(void* p)
{
    static uint8_t trials = 0; 
    // For ever loop
    for ( ; ; )
    {
        OSAL_TakeBinarySemaphore(&_periodicCheckTimeSemaphore);

        LOG_Trace("In periodic check time");

        if (periodicCheckTimer)
        {
            // Delete the existing timer
            OSAL_DestroyTimer(periodicCheckTimer);
            periodicCheckTimer = NULL;
        }

        if(!_timeIsValid)
        {
            EnsoPropertyValue_u testTime = LSD_GetTimeNow();
            if (testTime.timestamp.isValid)
            {
                // Time is now valid
                _timeAdjustment = testTime.timestamp.seconds - _timeAtStart;
                _timeIsValid = true;
                LOG_Info("Time now valid, time adjustment is %i", _timeAdjustment);

                uint16_t deltaCount = 0;
                LSD_SendPropertiesByFilter(TIMESTAMP_HANDLER, REPORTED_GROUP,
                        PROPERTY_FILTER_TIMESTAMPS, &deltaCount);
                LOG_Info("Found %i timestamp properties to test", deltaCount);
            }
            else
            {
                // Test again in 1 second
                periodicCheckTimer = OSAL_NewTimer(_PeriodicCheckTimeCB, 1000, false, NULL);
                trials++;
                if(trials>20)
                {
                    OSAL_watchdog_suspend_refresh();
                }
            }
        }
    }
}


