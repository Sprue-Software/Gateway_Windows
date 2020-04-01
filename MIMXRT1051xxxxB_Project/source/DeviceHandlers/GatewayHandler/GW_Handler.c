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
#include <stdlib.h>

#define RESET_WAIT_TIME_IN_MS           (10000)

// Minimum and maximum number of characters of the url for the Certificate Manager command
// Example: https://certmgr.intamac.com
#define CERT_MANAGER_URL_MIN         15
#define CERT_MANAGER_URL_MAX         100

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
 * \brief Calls the gateway certificate update script
 *
 * \param certificate manager url
 *
 * \return Success or Error code
 *
 */
static void _CertsUrl(char* url)
{
    if (!url ||
        strlen(url) < CERT_MANAGER_URL_MIN ||
        strlen(url) > CERT_MANAGER_URL_MAX)
    {
        LOG_Error("Url invalid");
        return;
    }

    const char certManagerScript[] = "KeyStore_Update";
    char certManagerCommand[CERT_MANAGER_URL_MAX + sizeof(certManagerScript) + 2];
    certManagerCommand[0] = '\0';
    strcat(certManagerCommand, certManagerScript);
    strcat(certManagerCommand, " ");
    strcat(certManagerCommand, url);

    if (system(certManagerCommand))
    {
        LOG_Error("KeyStore_Reset failed");
    }
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

/**
 * \brief   Reset Handler - receives messages on mq
 *
 * \param   mq Message queue on which to listen.
 */
void GW_Handler(MessageQueue_t mq)
{
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
                    case PROP_CERT_MANAGER_URL_ID:
                        {
                            LOG_Trace("Certificate Manager URL");
                            retVal = LSD_SetPropertyValueByAgentSideId(GW_HANDLER,
                                                                       &_gatewayDeviceId,
                                                                       REPORTED_GROUP,
                                                                       PROP_CERT_MANAGER_URL_ID,
                                                                       delta->propertyValue);
                            _CertsUrl(delta->propertyValue.memoryHandle);
                        }
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
