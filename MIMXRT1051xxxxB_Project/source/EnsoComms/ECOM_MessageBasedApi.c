/*!****************************************************************************
 *
 * \file ECOM_MessageBasedApi.c
 *
 * \author Evelyne Donnaes
 *
 * \brief Enso Communication API - Message-based Implementation
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdbool.h>
#include <string.h>

#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "LOG_Api.h"



/*!****************************************************************************
 * Constants
 *****************************************************************************/

// The maximum number of clients registering with Enso Comms
// These are the device handlers, the rules engine etc.
#define ECOM_MAXIMUM_NUMBER_OF_CLIENTS (ENSO_HANDLER_MAX)


/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/



/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

// Static arrays to store the clients message queue
static MessageQueue_t              _clientsMessageQueue[ECOM_MAXIMUM_NUMBER_OF_CLIENTS];


/******************************************************************************
 * Private Functions
 *****************************************************************************/


/******************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * \name ECOM_Init
 *
 * \brief Initialise the ECOM module
 */
void ECOM_Init(void)
{
    memset(_clientsMessageQueue, 0, sizeof _clientsMessageQueue);
}

/**
 * \name ECOM_GetMessageQueue
 *
 * \brief Returns the message queue handle associated with handlerID
 *        for the specified handler.
 *
 * \param  handlerId   The ID of the handler.
 *                     This is the first byte of the EnsoDeviceId_t
 *
 * \return The Message queue associated with the handlerID, NULL on error.
 */
MessageQueue_t ECOM_GetMessageQueue(const HandlerId_e handlerId)
{
    assert(handlerId < ECOM_MAXIMUM_NUMBER_OF_CLIENTS);
    // Return the message queue
    return _clientsMessageQueue[handlerId];
}

/**
 * \name ECOM_RegisterOnUpdateFunction
 *
 * \brief This function registers the update notifications callback function
 *        for the specified handler.
 *
 *        Note: This is required for the function based implementation only.
 *              For message passing implementation, this function does nothing
 *
 * \param  handlerId        The handler registering for update notifications.
 *                          This is the first byte of the EnsoDeviceId_t
 *
 * \param  onUpdateFunction Pointer to a function that is called to receive the
 *                          update notifications.
 *
 * \return                  ensoError
 *
 */
EnsoErrorCode_e ECOM_RegisterOnUpdateFunction(
        const HandlerId_e handlerId,
        const ECOM_OnUpdateNotification_t onUpdateFunction)
{
    return eecNoError;
}

/**
 * \name ECOM_RegisterMessageQueue
 *
 * \brief This function registers the message queue for the specified handler.
 *
 * \param  handlerId        The handler registering its message queue.
 *                          This is the first byte of the EnsoDeviceId_t
 *
 * \param  queue            The message queue for this handler.
 *
 * \return                  ensoError
 *
 */
EnsoErrorCode_e ECOM_RegisterMessageQueue(
        const HandlerId_e handlerId,
        const MessageQueue_t queue)
{
    if (handlerId >= ECOM_MAXIMUM_NUMBER_OF_CLIENTS)
    {
        LOG_Error("Failed to register handler %d, too many clients", handlerId);
        return eecParameterOutOfRange;
    }

    // Register this handler message queue
    _clientsMessageQueue[handlerId] = queue;

    return eecNoError;
}


/**
 * \name ECOM_SendUpdateToSubscriber
 *
 * \brief This function is called to notify a subscriber of a list of property
 *        change.
 *
 * \param  subscriberId       The subscriber id
 *
 * \param  publishedDeviceId  The thing that changed so is publishing the
 *                           update to its subscriber list
 *
 * \param  propertyGroup     The group to which the properties belong (reported
 *                           or desired)
 *
 * \param  numProperties     The number of properties in the delta
 *
 * \param  deltasBuffer      The list of properties that have been changed
 *
 * \return                   EnsoErrorCode_e
 */
EnsoErrorCode_e ECOM_SendUpdateToSubscriber(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer)
{
    /* Sanity check */
    if (propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecParameterOutOfRange;
    }
    if (!deltasBuffer)
    {
        return eecNullPointerSupplied;
    }
    if (numProperties == 0)
    {
        LOG_Warning("Deltas buffer is empty, nothing to do");
        // Nothing to do
        return eecNoError;
    }
    if (numProperties > ECOM_MAX_DELTAS)
    {
        LOG_Error("Too many properties in delta buffer");
        // Nothing to do
        return eecBufferTooBig;
    }

    // Prepare the message
    ECOM_DeltaMessage_t deltaMessage;
    deltaMessage.messageId = ECOM_DELTA_MSG;
    deltaMessage.destinationId = subscriberId;
    deltaMessage.deviceId = publishedDeviceId;
    deltaMessage.propertyGroup = propertyGroup;
    deltaMessage.numProperties = numProperties;
    memcpy(deltaMessage.deltasBuffer, deltasBuffer, numProperties * sizeof(EnsoPropertyDelta_t));

    EnsoErrorCode_e retVal = eecNoError;

    // Get the message queue for this thing
    MessageQueue_t destQueue = ECOM_GetMessageQueue(subscriberId);

    if (destQueue)
    {
        if (OSAL_SendMessage(destQueue, &deltaMessage, sizeof(ECOM_DeltaMessage_t), MessagePriority_medium) < 0)
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
 * \name ECOM_SendThingStatusToSubscriber
 *
 * \brief As the thing goes through its different stages of life cycle it
 * shall report them to its subscribers.
 *
 * \param  subscriberId      The destination handler to receive the notification
 *
 * \param  registeringThing  The thing that changed so is publishing the
 *                           update to its subscriber list
 *
 * \param  newThingStatus    The new status of the thing to pass on to the
 *                           subscriber
 *
 * \return                   EnsoErrorCode_e
 */
EnsoErrorCode_e ECOM_SendThingStatusToSubscriber(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t registeringThing,
        const EnsoDeviceStatus_e newThingStatus)
{

    EnsoErrorCode_e retVal = eecNoError;

    ECOM_ThingStatusMessage_t statusMessage = (ECOM_ThingStatusMessage_t) {
        .messageId = ECOM_THING_STATUS,
        .deviceId = registeringThing,
        .deviceStatus = newThingStatus
    };

    // Get the message queue for this thing
    MessageQueue_t destQueue = ECOM_GetMessageQueue(subscriberId);
    if (destQueue == NULL)
    {
        LOG_Error("Message queue for subscriberId = %d not found",subscriberId);
        retVal = eecInternalError;
    }
    else
    {
        if (OSAL_SendMessage(destQueue, &statusMessage, sizeof(ECOM_ThingStatusMessage_t), MessagePriority_medium) < 0)
        {
            retVal = eecInternalError;
            LOG_Error("OSAL_SendMessage failed %s", strerror(errno));
        }
        else
        {
            LOG_Info("ECOM_THING_STATUS sent");
        }
    }

    return retVal;
}


/**
 * \name ECOM_SendLocalShadowStatusToSubscriber
 *
 * \brief Sends the local shadow status to a destination handler.
 *
 * \param  destinationId     The destination handler to receive the notification
 *
 * \param  status            The local shadow status
 *
 * \param  propertyGroup     Property group to consider
 *
 * \return                   EnsoErrorCode_e
 */
EnsoErrorCode_e ECOM_SendLocalShadowStatusToSubscriber(
        const HandlerId_e destinationId,
        const LocalShadowStatus_e status,
        const PropertyGroup_e propertyGroup)
{
    EnsoErrorCode_e retVal = eecNoError;

    ECOM_LocalShadowStatusMessage_t statusMessage = (ECOM_LocalShadowStatusMessage_t) {
        .messageId = ECOM_LOCAL_SHADOW_STATUS,
        .shadowStatus = status,
        .propertyGroup = propertyGroup
    };

    // Get the message queue for this thing
    MessageQueue_t destQueue = ECOM_GetMessageQueue(destinationId);

    if (OSAL_SendMessage(destQueue, &statusMessage, sizeof(ECOM_LocalShadowStatusMessage_t), MessagePriority_medium) < 0)
    {
        retVal = eecInternalError;
        LOG_Error("OSAL_SendMessage failed %s", strerror(errno));
    }
    else
    {
        LOG_Info("ECOM_LOCAL_SHADOW_STATUS sent");
    }

    return retVal;
}


/**
 * \name ECOM_IsDestinationQueueFull
 *
 * \brief Check if the queue of a destination handler is full.
 *
 * \param  destinationId     The destination handler whose queue to check
 *
 * \return                   True if destination queue is full
 */
bool ECOM_IsDestinationQueueFull(const HandlerId_e destinationId)
{
    // Get the message queue for this thing
    MessageQueue_t destQueue = ECOM_GetMessageQueue(destinationId);

    return OSAL_GetMessageQueueNumCurrentMessages(destQueue) >= OSAL_GetMessageQueueSize(destQueue);
}


/**
 * \name ECOM_SendPropertyDeletedToSubscriber
 *
 * \brief Sends a property deleted indication to a destination handler.
 *
 * \param  destinationId     The destination handler to receive the notification
 *
 * \param  deviceId          The device the property belongs to
 *
 * \param  agentSideId       The property identifier
 *
 * \param  cloudSideId       The cloud property identifier
 *
 * \return                   EnsoErrorCode_e
 */
EnsoErrorCode_e ECOM_SendPropertyDeletedToSubscriber(
        const HandlerId_e destinationId,
        const EnsoDeviceId_t deviceId,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId)
{
    EnsoErrorCode_e retVal = eecNoError;

    ECOM_PropertyDeletedMessage_t statusMessage = (ECOM_PropertyDeletedMessage_t) {
        .messageId = ECOM_PROPERTY_DELETED,
        .deviceId = deviceId,
        .agentSideId = agentSideId
    };

    strncpy(statusMessage.cloudSideId, cloudSideId, LSD_PROPERTY_NAME_BUFFER_SIZE);

    // Get the message queue for this thing
    MessageQueue_t destQueue = ECOM_GetMessageQueue(destinationId);

    if (OSAL_SendMessage(destQueue, &statusMessage, sizeof(ECOM_PropertyDeletedMessage_t), MessagePriority_medium) < 0)
    {
        retVal = eecInternalError;
        LOG_Error("OSAL_SendMessage failed %s", strerror(errno));
    }
    else
    {
        LOG_Info("ECOM_PROPERTY_DELETED sent");
    }

    return retVal;
}
