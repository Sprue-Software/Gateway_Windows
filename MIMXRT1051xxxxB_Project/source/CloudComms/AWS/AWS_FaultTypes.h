#ifndef _AWS_FAULT_TYPES_H_
#define _AWS_FAULT_TYPES_H_

/*!****************************************************************************
*
* \file AWS_EventTypes.h
*
* \author Gavin Dolling
*
* \brief  Private definitions that are used by module and unit tests
*         Not considered to be part of the API
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include "LSD_Types.h"
#include "ECOM_Api.h"

/******************************************************************************
 * Type Definitions
 *****************************************************************************/

typedef struct
{
    bool inUse;
    bool readyToSend;
    HandlerId_e subscriberId;
    EnsoDeviceId_t deviceId;
    PropertyGroup_e propertyGroup;
    uint16_t numProperties;
    EnsoPropertyDelta_t buffer[ECOM_MAX_DELTAS];
} Delta_t;

#endif
