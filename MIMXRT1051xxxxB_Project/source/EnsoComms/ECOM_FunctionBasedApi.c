/*!****************************************************************************
 *
 * \file ECOM_FunctionBasedApi.c
 *
 * \author Evelyne Donnaes
 *
 * \brief Enso Communication API - Function-based Implementation
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

// Static arrays to store the OnUpdate functions
static ECOM_OnUpdateNotification_t _clientsOnUpdateNotification[ECOM_MAXIMUM_NUMBER_OF_CLIENTS];


/******************************************************************************
 * Private Functions
 *****************************************************************************/

ECOM_OnUpdateNotification_t GetOnUpdateFunction(const HandlerId_e handlerId)
{
    assert(handlerId < ECOM_MAXIMUM_NUMBER_OF_CLIENTS);

    // Return the message queue
    return _clientsOnUpdateNotification[handlerId];
}


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
    memset(_clientsOnUpdateNotification, 0, sizeof(_clientsOnUpdateNotification));
}

/**
 * \name ECOM_RegisterOnUpdateFunction
 *
 * \brief This function registers the update notifications callback function
 *        for the specified handler.
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
    if (handlerId >= ECOM_MAXIMUM_NUMBER_OF_CLIENTS)
    {
        LOG_Error("Failed to register handler %d, too many clients", handlerId);
        return eecPoolFull;
    }

    if (!onUpdateFunction)
    {
        LOG_Error("Invalid onUpdateFunction for handler %d", handlerId);
        return eecNullPointerSupplied;
    }

    // Register this handler onUpdateNotification function
    _clientsOnUpdateNotification[handlerId] = onUpdateFunction;

    return eecNoError;
}

/**
 * \name ECOM_RegisterMessageQueue
 *
 * \brief This function registers the message queue for the specified handler.
 *
 *        Note: This is required for the message based implementation only.
 *              For function based implementation, this function does nothing
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
    return eecNoError;
}


/**
 * \name ECOM_SendUpdateToSubscriber
 *
 * \brief This function is called to notify a subscriber of a list of property
 *        change.
 *
 * \param  subscriberId      The destination handler to receive the notification
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

    ECOM_OnUpdateNotification_t callback = GetOnUpdateFunction(subscriberId);

    if (callback)
    {
        callback(subscriberId, publishedDeviceId, propertyGroup, numProperties, deltasBuffer);
    }
    else
    {
        //LOG_Error("No callback registered");
    }

    return eecNoError;
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
 *                           update to its subscriber list.
 *
 * \param  newThingStatus    The new status of the thing to pass on to the
 *                           subscriber.
 *
 * \return                   EnsoErrorCode_e
 */
EnsoErrorCode_e ECOM_SendThingStatusToSubscriber(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t registeringThing,
        const EnsoDeviceStatus_e newThingStatus )
{
    EnsoErrorCode_e retVal = eecNoError;

    //LOG_Warning("Not implemented");

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

    LOG_Warning("Not implemented");

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
    return false;
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

    //LOG_Warning("Not implemented");

    return retVal;
}

