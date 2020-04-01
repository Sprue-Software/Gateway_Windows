#ifndef __LSD_API
#define __LSD_API

/*!****************************************************************************
 *
 * \file LSD_Api.h
 *
 * \author Carl Pickering
 *
 * \brief Interface to the local shadow.
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "LSD_Types.h"


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e LSD_Init(void);

EnsoErrorCode_e LSD_SyncLocalShadow(
        const HandlerId_e destinationHandler,
        const PropertyGroup_e propertyGroup,
        const uint16_t maxMessages,
        uint16_t* sentMessages);

EnsoErrorCode_e LSD_SendPropertiesByFilter(
        const PropertyFilter_e filter,
        const HandlerId_e destinationHandler,
        const PropertyGroup_e propertyGroup,
        uint16_t* sentDeltas);

EnsoObject_t* LSD_FindEnsoObjectByDeviceId(const EnsoDeviceId_t* deviceId);

EnsoErrorCode_e LSD_GetDevicesId(
        const HandlerId_e handlerId,
        EnsoDeviceId_t* buffer,
        const uint16_t  bufferElems,
        uint16_t* numDevices);

int LSD_DeviceIdCompare(const EnsoDeviceId_t* leftThing, const EnsoDeviceId_t* rightThing);

bool LSD_IsPropertyNested(
        const char* cloudName,
        char* parentName,
        char* childname);

EnsoPropertyValue_u LSD_GetTimeNow();

/*****************************************************************************/
/*                                                                           */
/* Object based functions                                                    */
/*                                                                           */
/*****************************************************************************/

EnsoObject_t* LSD_CreateEnsoObject(const EnsoDeviceId_t newId);

EnsoErrorCode_e LSD_DestroyEnsoDevice(const EnsoDeviceId_t deviceId);

EnsoErrorCode_e LSD_RegisterEnsoObject( EnsoObject_t* newObject);

EnsoErrorCode_e LSD_AnnounceEnsoObjects(bool* finished);

EnsoErrorCode_e LSD_SetDeviceStatus(
        const EnsoDeviceId_t theDevice,
        const EnsoDeviceStatus_e deviceStatus);

EnsoErrorCode_e LSD_CreateProperty(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId,
        const EnsoValueType_e type,
        const PropertyKind_e kind,
        const bool buffered,
        const bool persistent,
        const EnsoPropertyValue_u* groupValues);

EnsoErrorCode_e LSD_CreateProperties(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId[],
        const char* cloudSideId[],
        const EnsoValueType_e type[],
        const PropertyKind_e kind,
        const bool buffered,
        const bool persistent,
        EnsoPropertyValue_u groupValues[][PROPERTY_GROUP_MAX],
        uint32_t numProperties);

EnsoErrorCode_e LSD_RestoreProperty(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId,
        EnsoPropertyType_t propType,
        const EnsoPropertyValue_u* groupValues);

EnsoErrorCode_e LSD_RemovePropertyByAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSideId);

EnsoErrorCode_e LSD_RemovePropertyByCloudSideId(
        const EnsoDeviceId_t* deviceId,
        const char* cloudName);

EnsoErrorCode_e LSD_SetPropertyOutOfSync(
        const EnsoDeviceId_t* deviceId,
        const char* propName,
        const PropertyGroup_e propertyGroup);

/*****************************************************************************/
/*                                                                           */
/* Subscribe functions                                                       */
/*                                                                           */
/*****************************************************************************/

EnsoErrorCode_e LSD_SubscribeToDevice(
        const EnsoDeviceId_t* subjectDeviceId,
        PropertyGroup_e subjectPropertyGroup,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate);

EnsoErrorCode_e LSD_SubscribeToDevicePropertyByAgentSideId(
        const EnsoDeviceId_t* subjectDeviceId,
        EnsoAgentSidePropertyId_t subjectAgentPropId,
        PropertyGroup_e subjectPropertyGroup,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate);

/*****************************************************************************/
/*                                                                           */
/* Get functions                                                             */
/*                                                                           */
/*****************************************************************************/

EnsoErrorCode_e LSD_GetObjectPropertyValueByAgentSideId(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        EnsoPropertyValue_u* pRxValue);

EnsoErrorCode_e LSD_GetObjectPropertyBufferByAgentSideId(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        const size_t rxBufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied );

EnsoErrorCode_e LSD_GetPropertyCloudNameFromAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        const size_t bufferSize,
        char* cloudName);

EnsoProperty_t* LSD_GetPropertyByCloudName( // Should be removed ZZZ
        const EnsoDeviceId_t* deviceId,
        const char* cloudName);

EnsoErrorCode_e LSD_GetPropertyValueByCloudName(
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char* cloudName,
        EnsoPropertyValue_u* pRxValue);

EnsoProperty_t* LSD_GetPropertyByAgentSideId(  // Should be removed ZZZ
        const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSidePropertyId);

bool LSD_IsPropertyBufferedByAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSidePropertyId);

EnsoErrorCode_e LSD_GetPropertyValueByAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        EnsoPropertyValue_u* pRxValue);

EnsoErrorCode_e LSD_GetPropertyBufferByCloudName(
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char* cloudName,
        const size_t rxBufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied);

EnsoErrorCode_e LSD_GetPropertyBufferByAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        const size_t rxBufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied);

/*****************************************************************************/
/*                                                                           */
/* Set functions                                                             */
/*                                                                           */
/*****************************************************************************/

EnsoErrorCode_e LSD_SetPropertyValueByCloudName(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char* cloudName,
        const EnsoPropertyValue_u newValue);

EnsoErrorCode_e LSD_SetPropertyValueByCloudNameWithoutNotification(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char* cloudName,
        const EnsoPropertyValue_u newValue,
        EnsoPropertyDelta_t* delta,
        int* deltaCounter);

EnsoErrorCode_e LSD_SetPropertyValueByAgentSideId(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        const EnsoPropertyValue_u newValue);

EnsoErrorCode_e LSD_SetPropertyValueByAgentSideIdForObject(
        const HandlerId_e source,
        EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSideId,
        const EnsoPropertyValue_u newValue);

EnsoErrorCode_e LSD_SetPropertyBufferByCloudName(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char *cloudName,
        const size_t bufferSize,
        const void* pNewBuffer,
        size_t* pBytesCopied);

EnsoErrorCode_e LSD_SetPropertyBufferByAgentSideId(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        const size_t bufferSize,
        const void* pNewBuffer,
        size_t* pBytesCopied);

EnsoErrorCode_e LSD_SetPropertiesOfDevice(
        const HandlerId_e source,
        const EnsoDeviceId_t* destDevice,
        const PropertyGroup_e propertyGroup,
        const EnsoPropertyDelta_t* propertyDelta,
        const uint16_t numProperties);

void LSD_DumpObjectStore(void);

#endif
