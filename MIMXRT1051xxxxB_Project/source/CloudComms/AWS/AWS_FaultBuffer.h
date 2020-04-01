#ifndef _AWS_FAULT_BUFFER_H_
#define _AWS_FAULT_BUFFER_H_

/*!****************************************************************************
*
* \file AWS_FaultBuffer.h
*
* \author Gavin Dolling
*
* \brief  Buffer events to ensure they are reliably sent to AWS
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include "LSD_Types.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/



/******************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e AWS_FaultBuffer(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t deviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer);

void AWS_FinishedWithFaultBuffer(void* ptr);

void AWS_SendFailedForFaultBuffer(void);

EnsoErrorCode_e AWS_FaultBufferInit(void);

void AWS_FaultBufferStop(void);

bool AWS_FaultBufferEmpty(void);

void AWS_SendWaitingDelta(void);

void AWS_SetTimerForPeriodicWorker(void);

void AWS_ForceBackoffDuringDiscovery(void);

void AWS_ConnectionStateCB(bool connected);

#endif
