/*!****************************************************************************
 *
 * \file LEDH_Api.c
 *
 * \author Evelyne Donnaes
 *
 * \brief LED Handler Implementation
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "LEDH_Api.h"
#include "LOG_Api.h"
#include "LSD_Api.h"
#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "HAL.h"
#include "LSD_Api.h"
#include "APP_Types.h"
#include "SYS_Gateway.h"

#define LEDH_LEARN_LED_FLASH_DURATION_IN_MS     (500)

/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static void _LEDH_MessageQueueListener(MessageQueue_t mq);

static EnsoErrorCode_e _LEDH_OnLEDHandler(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer);

/******************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * \name LEDH_Initialise
 *
 * \brief Initialise the LED Handler
 *
 * \return EnsoErrorCode_e
 */
EnsoErrorCode_e LEDH_Initialise(void)
{
    // Create message queue for enso communications
    const char * name = "/ledDH";
    MessageQueue_t mq = OSAL_NewMessageQueue(name,
                                             ECOM_MAX_MESSAGE_QUEUE_DEPTH,
                                             ECOM_MAX_MESSAGE_SIZE);
    if (mq == NULL)
    {
        LOG_Error("OSAL_NewMessageQueue(\"%s\", %d, %d) failed", name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
        return eecInternalError;
    }
    // Create listening thread
    Thread_t listeningThread = OSAL_NewThread(_LEDH_MessageQueueListener, mq);
    if (listeningThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for ledDH");
        return eecInternalError;
    }

    ECOM_RegisterOnUpdateFunction(LED_DEVICE_HANDLER, _LEDH_OnLEDHandler);
    ECOM_RegisterMessageQueue(LED_DEVICE_HANDLER, mq);

    return eecNoError;
}


/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * \brief Flashes a led with a specified count
 *
 * \param led to be flashed LED no
 * \param flashCount count for flashing
 * \param flashDuratinInMs time between led states (on/off)
 *
 * \return none
 *
 */
static void LEDH_FlashLED(uint8_t led, uint32_t flashCount, uint32_t flashDurationInMs)
{
    for (int i = 0; i < flashCount; i++)
    {
        // Set the LED to the new value
        int16_t status = HAL_SetLed(led, HAL_LEDSTATE_OFF);
        if (status < 0)
        {
            LOG_Error("HAL_SetLed %d error %d", led, status);
        }

        OSAL_sleep_ms(flashDurationInMs);

        // Set the LED to the new value
        status = HAL_SetLed(led, HAL_LEDSTATE_ON);
        if (status < 0)
        {
            LOG_Error("HAL_SetLed %d error %d", led, status);
        }

        OSAL_sleep_ms(flashDurationInMs);
    }
}

/*
 * \brief Handler function for property change deltas on the object
 *
 * \param  subscriberId      The subscriber id
 *
 * \param  publishedDeviceId  The thing that changed so is publishing the
 *                           update to its subscriber list.
 *
 * \param  propertyGroup     The group to which the properties belong (reported
 *                           or desired)
 *
 * \param  numProperties     The number of properties in the delta
 *
 * \param  deltasBuffer      The list of properties that have been changed
 *
 * \return                      Error code
 */
static EnsoErrorCode_e _LEDH_OnLEDHandler(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer)
{
    EnsoErrorCode_e retVal = eecNoError;

    //LOG_Info("NumProperties %d", numProperties);

    // Sanity check
    assert(deltasBuffer);

    if (REPORTED_GROUP != propertyGroup)
    {
        LOG_Error("Expected REPORTED_GROUP, got %d instead", propertyGroup);
        return eecInternalError;
    }

    if (LED_DEVICE_HANDLER != subscriberId)
    {
        LOG_Error("Expected LED_DEVICE_HANDLER, got %d instead", subscriberId);
        return eecInternalError;
    }

    for (int i = 0; i < numProperties; i++)
    {
        EnsoAgentSidePropertyId_t propertyId = deltasBuffer[i].agentSidePropertyID;

        // Find out which LED the property corresponds to
        uint8_t led = HAL_NO_LEDS;
        uint8_t ledValue = HAL_LEDSTATE_OFF;
        switch (propertyId)
        {
            case PROP_ONLINE_ID:
                // The connection led is to be used to indicate the state of the cloud connection
                // Red if connection failed, otherwise not illuminated.
                if (!deltasBuffer[i].propertyValue.booleanValue)
                {
                    ledValue = HAL_LEDSTATE_ON;
                }
                led = HAL_CONNECTION_LED_ID;
                break;
            case PROP_LEARN_LED_FLASH_ID:
                {
                    /* Get flash count */
                    uint32_t count = deltasBuffer[i].propertyValue.uint32Value;

                    LEDH_FlashLED(HAL_POWER_LED_ID, count, LEDH_LEARN_LED_FLASH_DURATION_IN_MS);
                }
                continue;
            default:
                LOG_Error("Unexpected property %d", propertyId);
                continue;
        }

        // Set the LED to the new value
        int16_t status = HAL_SetLed(led, ledValue);
        if (status < 0)
        {
            LOG_Error("HAL_SetLed %d error %d", led, status);
            retVal = eecInternalError;
        }
    }

    return retVal;
}


/**
 * \brief   Consumer thread - receives messages on mq
 *
 * \param   mq Message queue on which to listen.
 */
static void _LEDH_MessageQueueListener(MessageQueue_t mq)
{
    LOG_Info("Starting LED Handler MessageQueueListener");

    // For ever loop
    for (; ; )
    {
        char buffer[ECOM_MAX_MESSAGE_SIZE];
        MessagePriority_e priority;

        int size = OSAL_ReceiveMessage(mq, buffer, sizeof(buffer), &priority);
        if (size < 1)
        {
            LOG_Error("ReceiveMessage error %d", size);
            continue;
        }

        // Read the message id
        uint8_t messageId = buffer[0];
        switch (messageId)
        {
            case ECOM_DELTA_MSG:
                //LOG_Info("*** LEDH received DELTA message");
                if (size != sizeof(ECOM_DeltaMessage_t))
                {
                    LOG_Error("Invalid DELTA message: size is %d, expected %d", size, sizeof(ECOM_DeltaMessage_t));
                }
                else
                {
                    ECOM_DeltaMessage_t* pDeltaMessage = (ECOM_DeltaMessage_t*)buffer;
                    _LEDH_OnLEDHandler(
                            pDeltaMessage->destinationId,
                            pDeltaMessage->deviceId,
                            pDeltaMessage->propertyGroup,
                            pDeltaMessage->numProperties,
                            pDeltaMessage->deltasBuffer);
                }
                break;

            default:
                LOG_Error("Unknown message %d", messageId);
                break;
        }
    }

    LOG_Warning("MessageQueueListener exit");
}
