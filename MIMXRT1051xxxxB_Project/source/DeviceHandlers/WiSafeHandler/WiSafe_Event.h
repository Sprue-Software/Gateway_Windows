/*!****************************************************************************
*
* \file WiSafe_Event.h
*
* \author James Green
*
* \brief WiSafe module that handles fault and alarm messages.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFE_EVENT_H_
#define _WISAFE_EVENT_H_

#include "WiSafe_RadioCommsBuffer.h"
#include "WiSafe_Protocol.h"
#include "WiSafe_Timer.h"

extern void WiSafe_FaultReceived(msgFault_t* msg);
extern void WiSafe_AlarmReceived(msgAlarm_t* msg);
extern void WiSafe_EventTimeout(timerType_e type, radioCommsBuffer_t* pendingFaultBuffer);
extern void WiSafe_ExtDiagVarRespReceived(msgExtDiagVarResp_t* msg);
extern void WiSafe_FaultDetailReceived(msgFaultDetail_t* msg);
extern void WiSafe_FaultAlarmStopReceived(msgAlarmStop_t* msg);
extern void WiSafe_FaultRMFaultReceived(radioCommsBuffer_t* buf);
extern void WiSafe_FaultRemoteStatusReportReceived(radioCommsBuffer_t* buf);

extern void WiSafe_FaultHushReceived(msgHush_t* msg);	//////// [RE:decl] Added this
extern void WiSafe_FaultLocateReceived(msgLocate_t* msg);	//////// [RE:decl] Added this

#endif /* _WISAFE_EVENT_H_ */
