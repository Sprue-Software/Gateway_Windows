#ifndef __LSD_PROPERTYSTORE
#define __LSD_PROPERTYSTORE

/*!****************************************************************************
 *
 * \file LSD_PropertyStore.h
 *
 * \author Carl Pickering
 *
 * \brief The property store for the ensoAgent
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "LSD_Types.h"
#include "LSD_EnsoObjectStore.h"
#include "EnsoConfig.h"

/*!****************************************************************************
 * Constants
 *****************************************************************************/

#define LSD_MAX_TRANSFER_DELTAS (32)

/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/


/**
 * \name EnsoPropertyContainer_t
 *
 * \brief Enso Property container
 *
 * This structure is used to contain a pointer to a property so we may
 * create linked lists etc in arrays. The intention is to save space compared
 * with pointers and make sense when passed across process boundaries.
 *
 */
typedef struct EnsoPropertyContainer_tag
{
    EnsoProperty_t property;
    EnsoIndex_t nextContainerIndex;
} EnsoPropertyContainer_t;

/**
 * \name EnsoPropertyStore_t
 *
 * \brief This contains the property pool.
 *
 * NOT THREAD SAFE
 */

typedef struct EnsoPropertyStore_tag
{
    EnsoIndex_t firstFreeProperty;
    EnsoPropertyContainer_t propertyPool[LSD_PROPERTY_POOL_SIZE];
} EnsoPropertyStore_t;

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e LSD_InitialisePropertyStore(Mutex_t * mutex);

EnsoErrorCode_e LSD_SetPropertyOutOfSyncDirectly(
        const EnsoDeviceId_t* deviceId,
        const char* propName,
        const PropertyGroup_e propertyGroup);

EnsoErrorCode_e LSD_CreatePropertyDirectly(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId,
        const EnsoValueType_e type,
        const PropertyKind_e kind,
        const bool buffered,
        const bool persistent,
        const EnsoPropertyValue_u* groupValues);

EnsoErrorCode_e LSD_RemoveProperty_Safe(
        const EnsoDeviceId_t* deviceId,
        const bool removeFirstProperty,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudName,
        const bool notify);

EnsoErrorCode_e  LSD_RemoveAllPropertiesOfObject_Safe(
        const EnsoDeviceId_t* deviceId);

EnsoPropertyValue_u* LSD_GetPropertyValuePtrFromGroupDirectly(
        const EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup );;

EnsoProperty_t* LSD_FindPropertyByCloudSideIdDirectly(
        const EnsoObject_t* owner,
        const char* cloudSideId);

EnsoProperty_t* LSD_FindPropertyByAgentSideIdDirectly(
        const EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId);

EnsoErrorCode_e LSD_SetPropertyValueByAgentSideIdDirectly(
        EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSideId,
        const EnsoPropertyValue_u newValue);

EnsoErrorCode_e LSD_CopyBufferToPropertyDirectly(
        EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup,
        const size_t bufferSize,
        const void* newBuffer);

EnsoErrorCode_e LSD_CopyFromPropertyBufferByAgentSideIdDirectly(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSideId,
        const size_t bufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied);

EnsoErrorCode_e LSD_GetSizeOfPropertyBufferDirectly(
        const EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup,
        size_t* bufferSize);

EnsoErrorCode_e LSD_GetSizeOfPropertyBufferContentDirectly(
        const EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup,
        size_t* bufferSize);

EnsoErrorCode_e LSD_GetPropertyValueByCloudSideIdDirectly(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const char* cloudSideId,
        EnsoPropertyValue_u* pRxValue);

EnsoErrorCode_e LSD_CopyFromPropertyBufferByCloudSideIdDirectly(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const char* cloudSideId,
        const size_t bufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied);

uint16_t LSD_SetPropertiesOfObjectDirectly(
        EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoPropertyDelta_t* propertyDelta,
        const uint16_t numProperties);

EnsoErrorCode_e LSD_SendPropertiesByFilter_Safe(
        const PropertyFilter_e filter,
        EnsoObject_t* owner,
        const HandlerId_e destination,
        const PropertyGroup_e propertyGroup,
        const uint16_t maxDeltas,
        uint16_t* numDeltas);

void LSD_DumpProperties(int index);

#endif
