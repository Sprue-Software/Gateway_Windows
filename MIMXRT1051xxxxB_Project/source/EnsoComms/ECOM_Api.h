#ifndef _ECOM_API_H_
#define _ECOM_API_H_

/*!****************************************************************************
 *
 * \file ECOM_Api.h
 *
 * \author Evelyne Donnaes
 *
 * \brief Enso Communication Layer
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "OSAL_Api.h"
#include "LSD_Types.h"

/*!****************************************************************************
 * Constants
 *****************************************************************************/

// The maximum message size
#define ECOM_MAX_MESSAGE_SIZE (192)

// The maximum message queue depth
#define ECOM_MAX_MESSAGE_QUEUE_DEPTH (20)

// The maximum number of deltas in a DELTA message, will need
// to be reduced if LSD_STRING_PROPERTY_MAX_LENGTH is increased
#define ECOM_MAX_DELTAS (6)

/*!****************************************************************************
 * Types
 *****************************************************************************/

/**
 * \name ECOM_OnUpdateNotification_t
 *
 * \brief Callback function to report changes on a set of properties.
 *
 * \param  subscriberId      The subscriber id
 *
 * \param  publishedDeviceId  The thing that changed so is publishing the
 *                           update to its subscriber list.
 *
 * \param propertyGroup - The group to which the property belongs, currently
 * reported and desired group.
 *
 * \param numPropertiesChanged - The number of properties that are contained in
 * changedPropertyList.
 *
 * \param changedPropertyList - The list of properties that have been changed.
 */
typedef EnsoErrorCode_e (*ECOM_OnUpdateNotification_t)( const HandlerId_e subscriberId,
                                                        const EnsoDeviceId_t publishedDeviceId,
                                                        const PropertyGroup_e propertyGroup,
                                                        const uint16_t numPropertiesChanged,
                                                        const EnsoPropertyDelta_t* deltasBuffer);


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

void ECOM_Init(void);

MessageQueue_t ECOM_GetMessageQueue(const HandlerId_e handlerId);

EnsoErrorCode_e ECOM_RegisterOnUpdateFunction(
        const HandlerId_e handlerId,
        const ECOM_OnUpdateNotification_t onUpdateFunction);

EnsoErrorCode_e ECOM_RegisterMessageQueue(
        const HandlerId_e handlerId,
        const MessageQueue_t queue);

EnsoErrorCode_e ECOM_SendUpdateToSubscriber(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer);

EnsoErrorCode_e ECOM_SendThingStatusToSubscriber(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t registeringThing,
        const EnsoDeviceStatus_e newThingStatus);

EnsoErrorCode_e ECOM_SendLocalShadowStatusToSubscriber(
        const HandlerId_e destinationId,
        const LocalShadowStatus_e status,
        const PropertyGroup_e propertyGroup);

EnsoErrorCode_e ECOM_SendPropertyDeletedToSubscriber(
        const HandlerId_e destinationId,
        const EnsoDeviceId_t deviceId,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId);

bool ECOM_IsDestinationQueueFull(const HandlerId_e destinationId);

#endif
