/*!****************************************************************************
*
* \file WiSafe_Main.c
*
* \author James Green
*
* \brief Main entry point and message loop for WiSafe device handler.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

//#include <unistd.h>
#include <signal.h>
#include <ctype.h>
//#include <sys/time.h>
#include <stdlib.h>

#include "LOG_Api.h"
#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "LSD_Types.h"
#include "LSD_Api.h"

#include "WiSafe_RadioComms.h"
#include "WiSafe_Main.h"
#include "WiSafe_Engine.h"
#include "WiSafe_Discovery.h"
#include "WiSafe_DAL.h"
#include "WiSafe_Event.h"
#include "WiSafe_Timer.h"

MessageQueue_t MainMessageQueue;

/**
 * Helper to create the standard properties of the gateway device.
 *
 */
static void CreateGatewayProperties(void)
{
    EnsoPropertyValue_u valueZ = { .uint32Value = 0 };
    EnsoPropertyValue_u valueF = { .booleanValue = false };

    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_IDX, (propertyName_t) DAL_COMMAND_TEST_MODE, evUnsignedInt32, valueZ);
    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_FLUSH_IDX, (propertyName_t) DAL_COMMAND_TEST_MODE_FLUSH, evBoolean, valueF);

    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_IDX, (propertyName_t) DAL_COMMAND_LEARN, evUnsignedInt32, valueZ);
    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_TIMEOUT_IDX, (propertyName_t) DAL_COMMAND_LEARN_TIMEOUT, evUnsignedInt32, valueZ);

    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_FLUSH_IDX, (propertyName_t) DAL_COMMAND_FLUSH, evUnsignedInt32, valueZ);

    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, (propertyName_t)DAL_PROPERTY_IN_NETWORK, evBoolean, valueF);

    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_DELETE_ALL_IDX, (propertyName_t)DAL_COMMAND_DELETE_ALL, evUnsignedInt32, valueZ);
    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_SOUNDER_TEST_IDX, (propertyName_t)DAL_COMMAND_SOUNDER_TEST, evUnsignedInt32, valueZ);
    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_SILENCE_ALL_IDX, (propertyName_t)DAL_COMMAND_SILENCE_ALL, evUnsignedInt32, valueZ);
    WiSafe_DALCreateProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LOCATE_IDX, (propertyName_t)DAL_COMMAND_LOCATE, evUnsignedInt32, valueZ);
}

/**
 * Helper to subscribe for deltas to desired properties so that we can
 * receive commands via properties from the platform.
 *
 */
static void SubscribeToCommandProperties(void)
{
    EnsoErrorCode_e error = WiSafe_DALSubscribeToDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_TEST_MODE %s", LSD_EnsoErrorCode_eToString(error));
    }

    error = WiSafe_DALSubscribeToDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_LEARN %s", LSD_EnsoErrorCode_eToString(error));
    }

    error = WiSafe_DALSubscribeToDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_FLUSH_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_FLUSH %s", LSD_EnsoErrorCode_eToString(error));
    }

    error = WiSafe_DALSubscribeToDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_DELETE_ALL_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_DELETE_ALL %s", LSD_EnsoErrorCode_eToString(error));
    }

    error = WiSafe_DALSubscribeToDesiredProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to PROP_STATE %s", LSD_EnsoErrorCode_eToString(error));
    }

    error = WiSafe_DALSubscribeToDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_SOUNDER_TEST_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_SOUNDER_TEST %s", LSD_EnsoErrorCode_eToString(error));
    }

    error = WiSafe_DALSubscribeToDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_SILENCE_ALL_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_SILENCE_ALL %s", LSD_EnsoErrorCode_eToString(error));
    }

    error = WiSafe_DALSubscribeToDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LOCATE_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_LOCATE %s", LSD_EnsoErrorCode_eToString(error));
    }
}

/**
 * Helper to listen and process messages received on our message queue.
 *
 * @param mq The message queue
 */
static void WiSafe_MessageQueueListener(MessageQueue_t mq)
{
    LOG_Info("Starting WiSafe Handler MessageQueueListener");

    /* Loop forever. */
    while (true)
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
            /* Handle property deltas. */
            case ECOM_DELTA_MSG:
            {
                if (size != sizeof(ECOM_DeltaMessage_t))
                {
                    LOG_Error("Invalid DELTA message: size is %d, expected %d", size, sizeof(ECOM_DeltaMessage_t));
                }
                else
                {
                    /* Parse the delta message and extract what we need. */
                    ECOM_DeltaMessage_t* deltaMessage = (ECOM_DeltaMessage_t*)buffer;

                    /* Check this is a change to a desired property. */
                    if (deltaMessage->propertyGroup != DESIRED_GROUP)
                    {
                        LOG_Error("Expected DESIRED_GROUP, got %d instead", deltaMessage->propertyGroup);
                    }
                    else
                    {
                        /* Check the message is for us. */
                        if (deltaMessage->destinationId != WISAFE_DEVICE_HANDLER)
                        {
                            LOG_Error("Expected WISAFE_DEVICE_HANDLER, got %d instead", deltaMessage->destinationId);
                        }
                        else
                        {
                            /* Log what we've been given to the console. */
                            for (int idx = 0; idx < deltaMessage->numProperties; idx += 1)
                            {
                                EnsoPropertyDelta_t* delta = &deltaMessage->deltasBuffer[idx];
                                char propName[LSD_PROPERTY_NAME_BUFFER_SIZE] = {0,};
                                LSD_GetPropertyCloudNameFromAgentSideId(&(deltaMessage->deviceId), delta->agentSidePropertyID, sizeof(propName), propName);
                                LOG_InfoC(LOG_BLUE "Received property (%d/%d) delta on device %016llx for key '%s' (%x).", (idx + 1), deltaMessage->numProperties, deltaMessage->deviceId.deviceAddress, propName, delta->agentSidePropertyID);
                            }

                            /* Is it a single property or an atomic group? */
                            if (deltaMessage->numProperties == 1)
                            {
                                /* Single property change. */
                                EnsoPropertyDelta_t* delta = &deltaMessage->deltasBuffer[0];

                                /* Get the property name for debugging. */
                                char propName[LSD_PROPERTY_NAME_BUFFER_SIZE] = {0,};
                                LSD_GetPropertyCloudNameFromAgentSideId(&(deltaMessage->deviceId), delta->agentSidePropertyID, sizeof(propName), propName);
                                LOG_InfoC(LOG_BLUE "Processing property (1/1) delta on device %016llx for key '%s' (%x).", deltaMessage->deviceId.deviceAddress, propName, delta->agentSidePropertyID);

                                /* Call the engine to process it. */
                                WiSafe_EngineProcessPropertyDelta(&deltaMessage->deviceId, delta);
                            }
                            else
                            {
                                /* Atomic group change. */
                                WiSafe_EngineProcessGroupDelta(&deltaMessage->deviceId, deltaMessage->deltasBuffer, deltaMessage->numProperties);

                            }
                        }
                    }
                }
                break;
            }

            /* Handle discovery timeouts. */
            case ECOM_GENERAL_PURPOSE1:
            {
                timerMsg_GeneralPurpose1_st* msg = (timerMsg_GeneralPurpose1_st*)buffer;
                WiSafe_DiscoveryHandleTimer(msg->timerType);
                break;
            }

            /* Handle fault timeouts. */
            case ECOM_GENERAL_PURPOSE2:
            {
                timerMsg_GeneralPurpose2_3_st* msg = (timerMsg_GeneralPurpose2_3_st*)buffer;
                WiSafe_EventTimeout(timerType_FaultTimeout, msg->buffer);
                break;
            }

            /* Handle alarm timeouts. */
            case ECOM_GENERAL_PURPOSE3:
            {
                timerMsg_GeneralPurpose2_3_st* msg = (timerMsg_GeneralPurpose2_3_st*)buffer;
                WiSafe_EventTimeout(timerType_AlarmTimeout, msg->buffer);
                break;
            }

            /* Process any received data over SPI from the RM. */
            case ECOM_BUFFER_RX:
            {
                radioComms_bufferRx_st* msg = (radioComms_bufferRx_st*)buffer;
                WiSafe_EngineProcessRX(msg->buffer);
                WiSafe_RadioCommsBufferRelease(msg->buffer);
                break;
            }

            /* Process any received commands. */
            case ECOM_COMMAND_RX:
            {
                controlEntry_t* control = ((commandMessage_st*)buffer)->control;
                WiSafe_EngineProcessControlMessage(control);
                radioCommsBuffer_t* controlBuffer = (radioCommsBuffer_t*)control;
                WiSafe_RadioCommsBufferRelease(controlBuffer);
                break;
            }

            default:
                LOG_Error("Unknown message %d", messageId);
                break;
        }
    }

    LOG_Warning("WiSafe Handler exit");
}

/**
 * This is the main entry point for the WiSafe device handler.
 *
 * @return
 */
EnsoErrorCode_e WiSafe_Start(void)
{
    LOG_Info("WiSafe Device Handler started.");
    char * ptr = NULL;
    testMissingNormalPeriod = strtol(getenvDefault("testMissingNormalPeriod", "18h"), &ptr, 0);
    char c = tolower(*ptr);
    if (c == 'h')
    {
        testMissingNormalPeriod *= 60 * 60;
    }
    else if (c == 'm')
    {
        testMissingNormalPeriod *= 60;
    }
    LOG_Trace("testMissingNormalPeriod = %d", testMissingNormalPeriod);
    /* Create and register our message queue handler. */
    const char * name = "/WiSafeDH";
    MainMessageQueue = OSAL_NewMessageQueue(name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);

    Thread_t listeningThread = OSAL_NewThread(WiSafe_MessageQueueListener, MainMessageQueue);
    if (listeningThread == NULL)
    {
        LOG_Error("OSAL_NewThread failed for WiSafe Device Handler");
        return eecInternalError;
    }

    ECOM_RegisterMessageQueue(WISAFE_DEVICE_HANDLER, MainMessageQueue);

    /* Initialise the DAL. */
    WiSafe_DALInit();

    /* Initialise the packet receiver. */
    WiSafe_RadioCommsInit();

    /* Set up the shadow properties of the gateway device. */
    CreateGatewayProperties();

    /* Subscribe to control properties. */
    SubscribeToCommandProperties();

    /* Initialise the Discovery helper. */
    WiSafe_DiscoveryInit();

    return eecNoError;
}

/**
 * This function is called to free the resource of the WiSafe device
 * handler. Currently there is no support for calling this.
 *
 */
void WiSafe_End(void)
{
    /* Tidy. */
    LOG_Info("WiSafe Device Handler stopping.");
    WiSafe_RadioCommsClose();
    WiSafe_DALClose();
}

/**
 * This method handles timer callbacks for general purpose timers.
 *
 * @param handle The handle paassed when the callback was originally registered.
 */
void WiSafe_TimerCallback(void* handle)
{
    timerMsg_GeneralPurpose1_st msg;
    msg.messageType = ECOM_GENERAL_PURPOSE1;
    msg.timerType = (timerType_e)((Handle_t)OSAL_GetTimerParam(handle));

    const MessagePriority_e priority = MessagePriority_high;
    if (OSAL_SendMessage(MainMessageQueue, &msg, sizeof(msg), priority) < 0)
    {
        LOG_Error("SendMessage() failed.");
    }
}

/**
 * This method handles timer callbacks for fault timeouts.
 *
 * @param handle The handle paassed when the callback was originally registered.
 */
void WiSafe_FaultTimerCallback(void* handle)
{
    timerMsg_GeneralPurpose2_3_st msg;
    msg.messageType = ECOM_GENERAL_PURPOSE2;
    msg.buffer = (radioCommsBuffer_t*)((Handle_t)OSAL_GetTimerParam(handle));

    const MessagePriority_e priority = MessagePriority_high;
    if (OSAL_SendMessage(MainMessageQueue, &msg, sizeof(msg), priority) < 0)
    {
        LOG_Error("SendMessage() failed.");
    }
}

/**
 * This method handles timer callbacks for alarm timeouts.
 *
 * @param handle The handle paassed when the callback was originally registered.
 */
void WiSafe_AlarmTimerCallback(void* handle)
{
    timerMsg_GeneralPurpose2_3_st msg;
    msg.messageType = ECOM_GENERAL_PURPOSE3;
    msg.buffer = (radioCommsBuffer_t*)((Handle_t)OSAL_GetTimerParam(handle));

    const MessagePriority_e priority = MessagePriority_high;
    if (OSAL_SendMessage(MainMessageQueue, &msg, sizeof(msg), priority) < 0)
    {
        LOG_Error("SendMessage() failed.");
    }
}
