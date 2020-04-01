/*!****************************************************************************
*
* \file WiSafe_Engine.h
*
* \author James Green
*
* \brief WiSafe core engine that processes received messages and timers.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFEENGINE_H_
#define _WISAFEENGINE_H_

#include <stdint.h>

#include "OSAL_Api.h"

#include "WiSafe_RadioCommsBuffer.h"
#include "WiSafe_Timer.h"
#include "WiSafe_Control.h"
#include "WiSafe_DAL.h"

extern void WiSafe_EngineProcessRX(radioCommsBuffer_t* buf);
extern void WiSafe_EngineProcessControlMessage(controlEntry_t* message);
extern void WiSafe_EngineProcessPropertyDelta(EnsoDeviceId_t* deviceId, EnsoPropertyDelta_t* delta);
extern void WiSafe_EngineProcessGroupDelta(EnsoDeviceId_t* deviceId, EnsoPropertyDelta_t deltas[], int numProperties);

#endif /* _WISAFEENGINE_H_ */
