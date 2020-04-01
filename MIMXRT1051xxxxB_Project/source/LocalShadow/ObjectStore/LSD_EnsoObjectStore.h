#ifndef __LSD_ENSOOBJECTSTORE
#define __LSD_ENSOOBJECTSTORE

/*!****************************************************************************
*
* \file LSD_EnsoObjectStore.h
*
* \author Carl Pickering
*
* \brief The property store for the ensoObjects
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stdint.h>
#include "LSD_Types.h"


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e LSD_EnsoObjectStoreInit(void);

EnsoObject_t* LSD_CreateEnsoObjectDirectly(const EnsoDeviceId_t newId);

EnsoErrorCode_e LSD_DestroyEnsoDeviceDirectly(const EnsoDeviceId_t deviceId);

EnsoObject_t* LSD_FindEnsoObjectByDeviceIdDirectly(const EnsoDeviceId_t* deviceId);

EnsoObject_t* LSD_FindEnsoObjectNeedingAnnounceDirectly(void);

EnsoObject_t* LSD_GetNotUnsubcribedEnsoObjects(void);

int LSD_CountEnsoObjectsDirectly( void );

EnsoErrorCode_e LSD_RegisterEnsoObjectDirectly(EnsoObject_t* newObject);

EnsoErrorCode_e LSD_NotifyDirectly(
        const HandlerId_e source,
        const EnsoObject_t* destObject,
        const PropertyGroup_e propertyGroup,
        const EnsoPropertyDelta_t* deltas,
        const uint16_t numProperties);

EnsoErrorCode_e LSD_SetDeviceStatusDirectly(
        const EnsoDeviceId_t theDevice,
        const EnsoDeviceStatus_e deviceStatus);

bool LSD_IsEnsoDeviceIdValid(const EnsoDeviceId_t theDevice);

EnsoErrorCode_e LSD_CopyDeviceId(EnsoDeviceId_t* dest, const EnsoDeviceId_t* source);

EnsoErrorCode_e LSD_GetDevicesIdDirectly(
        const HandlerId_e handlerId,
        EnsoDeviceId_t* buffer,
        const uint16_t  bufferElems,
        uint16_t* numDevices);

#endif
