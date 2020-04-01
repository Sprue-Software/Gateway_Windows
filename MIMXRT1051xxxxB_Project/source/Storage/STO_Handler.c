/*!****************************************************************************
 *
 * \file STO_Handler.c
 *
 * \author Evelyne Donnaes
 *
 * \brief Storage Handler Implementation
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdio.h>
//#include <inttypes.h>
#include "STO_Handler.h"
#include "STO_Manager.h"
#include "LOG_Api.h"
#include "LSD_Api.h"
#include <ECOM_Api.h>
#include <ECOM_Messages.h>
#include "HAL.h"
#include "LSD_Api.h"
#include "SYS_Gateway.h"

/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static void _STO_MessageQueueListener(MessageQueue_t mq);

static EnsoErrorCode_e _STO_OnUpdate(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer);


/******************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * \name STO_Handler_Init
 *
 * \brief Initialise Storage Handler
 *
 * \return EnsoErrorCode_e
 */
EnsoErrorCode_e STO_Handler_Init(void)
{
    // Create message queue for enso communications
    const char * name = "/stoH";
    MessageQueue_t mq = OSAL_NewMessageQueue(name,
                                             ECOM_MAX_MESSAGE_QUEUE_DEPTH,
                                             ECOM_MAX_MESSAGE_SIZE);
    if (mq == NULL)
    {
        LOG_Error("OSAL_NewMessageQueue(\"%s\", %d, %d) failed", name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
        return eecInternalError;
    }

    // Create listening thread
    Thread_t listeningThread = OSAL_NewThread(_STO_MessageQueueListener, mq);
    if (listeningThread == NULL)
    {
        LOG_Error("OSAL_NewThread failed for stoH");
        return eecInternalError;
    }

    ECOM_RegisterOnUpdateFunction(STORAGE_HANDLER, _STO_OnUpdate);
    ECOM_RegisterMessageQueue(STORAGE_HANDLER, mq);

    // Initialise storage manager
    EnsoErrorCode_e retVal = STO_Init();

    return retVal;
}


/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

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
static EnsoErrorCode_e _STO_OnUpdate(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t deviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer)
{
    EnsoErrorCode_e retVal = eecNoError;

    //LOG_Info("NumProperties %d", numProperties);

    // Sanity check
    assert(deltasBuffer);

    for (int i = 0; i < numProperties; i++)
    {
        EnsoAgentSidePropertyId_t propertyId = deltasBuffer[i].agentSidePropertyID;

        EnsoProperty_t* property = LSD_GetPropertyByAgentSideId(&deviceId, propertyId);
        if (property)
        {
            LOG_Trace("Writing for %s, desired outOfSync=%i, reportedOutOfSync=%i",
                    property->cloudName, property->type.desiredOutOfSync,
                    property->type.reportedOutOfSync);

            // Write the new property value to the store
            EnsoTag_t tag;
            tag.deviceId = deviceId;
            tag.propId = propertyId;
            tag.propType = property->type;
            tag.propGroup = propertyGroup;
            retVal = STO_WriteRecord(&tag, property->cloudName, sizeof(EnsoPropertyValue_u), (EnsoPropertyValue_u*)&deltasBuffer[i].propertyValue);
            if (eecNoError != retVal)
            {
                // We could have a corrupted log. How shall we proceed?
                LOG_Error("FATAL: STO_WriteRecord error %s", LSD_EnsoErrorCode_eToString(retVal));
                break;
            }
        }
        else
        {
#if 0
            LOG_Error("Failed to find property %08x for %016"PRIx64"_%04x_%02x", deltasBuffer[i].agentSidePropertyID,
                      deviceId.deviceAddress, deviceId.technology, deviceId.childDeviceId);
#endif
        }
    }

    if (eecNoError == retVal)
    {
        retVal = STO_CheckSizeAndConsolidate();
        if (eecNoError != retVal)
        {
            LOG_Error("STO_CheckSizeAndConsolidate error %s", LSD_EnsoErrorCode_eToString(retVal));
        }
    }

    return retVal;
}


/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * \brief   Consumer thread - receives messages on mq
 *
 * \param   mq Message queue on which to listen.
 */
static void _STO_MessageQueueListener(MessageQueue_t mq)
{
    LOG_Info("Starting Storage Handler MessageQueueListener");

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
                LOG_Trace("Delta");
                if (size != sizeof(ECOM_DeltaMessage_t))
                {
                    LOG_Error("Invalid DELTA message: size is %d, expected %d", size, sizeof(ECOM_DeltaMessage_t));
                }
                else
                {
                    ECOM_DeltaMessage_t* pDeltaMessage = (ECOM_DeltaMessage_t*)buffer;
                    _STO_OnUpdate(
                            pDeltaMessage->destinationId,
                            pDeltaMessage->deviceId,
                            pDeltaMessage->propertyGroup,
                            pDeltaMessage->numProperties,
                            pDeltaMessage->deltasBuffer);
                }
                break;

            case ECOM_PROPERTY_DELETED:
                 LOG_Trace("Property Deleted");
                 if (size != sizeof(ECOM_PropertyDeletedMessage_t))
                 {
                     LOG_Error("Invalid ECOM_PROPERTY_DELETED message: size is %d, expected %d", size, sizeof(ECOM_PropertyDeletedMessage_t));
                 }
                 else
                 {
                     ECOM_PropertyDeletedMessage_t* pMessage = (ECOM_PropertyDeletedMessage_t*)buffer;

                     // A deleted property is indicated with a length of zero bytes. The tag is normal.
                     EnsoTag_t tag;
                     tag.deviceId = pMessage->deviceId;
                     tag.propId = pMessage->agentSideId;

                     EnsoPropertyValue_u propertyValue = { .uint32Value = 0 };
                     EnsoErrorCode_e retVal = STO_WriteRecord(&tag, "", 0, &propertyValue);
                     if (eecNoError != retVal)
                     {
                         // We could have a corrupted log.
                         LOG_Error("FATAL: STO_WriteRecord error %s deleting a property", LSD_EnsoErrorCode_eToString(retVal));
                     }
                 }
                 break;

            case ECOM_THING_STATUS:
                LOG_Info("*** ECOM_THING_STATUS message received");
                if (size != sizeof(ECOM_ThingStatusMessage_t))
                {
                    LOG_Error("Invalid ECOM_THING_STATUS message: size is %d, expected %d", size, sizeof(ECOM_ThingStatusMessage_t));
                }
                else
                {
                    ECOM_ThingStatusMessage_t* pMessage = (ECOM_ThingStatusMessage_t*)buffer;

                    if (THING_DELETED == pMessage->deviceStatus)
                    {
                        // A deleted device is indicated with a tag of the device Id but
                        // a property Id of zero (invalid property).
                        EnsoTag_t tag;
                        tag.deviceId = pMessage->deviceId;
                        tag.propId = 0;

                        EnsoPropertyValue_u propertyValue = { .uint32Value = 0 };
                        EnsoErrorCode_e retVal = STO_WriteRecord(&tag, "", 0, &propertyValue);
                        if (eecNoError != retVal)
                        {
                            // We could have a corrupted log.
                            LOG_Error("FATAL: STO_WriteRecord error %s deleting a device", LSD_EnsoErrorCode_eToString(retVal));
                        }
                    }
                }
                break;

            case ECOM_LOCAL_SHADOW_STATUS:
                LOG_Trace("Shadow Status");
                if (size != sizeof(ECOM_LocalShadowStatusMessage_t))
                {
                    LOG_Error("Invalid LOCAL_SHADOW_STATUS message: size is %d, expected %d", size, sizeof(ECOM_LocalShadowStatusMessage_t));
                }
                else
                {
                    ECOM_LocalShadowStatusMessage_t* pMessage = (ECOM_LocalShadowStatusMessage_t*) buffer;
                    if (pMessage->shadowStatus == LSD_DUMP_COMPLETED)
                    {
                        STO_LocalShadowDumpComplete(pMessage->propertyGroup);
                    }
                    else
                    {
                        LOG_Error("Unexpected local shadow status %d", pMessage->shadowStatus);
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
