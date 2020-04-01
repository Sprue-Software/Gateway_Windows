/*!****************************************************************************
*
* \file WiSafe_Main.h
*
* \author James Green
*
* \brief Main entry point and message loop for WiSafe device handler.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFEMAIN_H_
#define _WISAFEMAIN_H_

#include "WiSafe_RadioCommsBuffer.h"
#include "WiSafe_Timer.h"
#include "WiSafe_Control.h"
#include "WiSafe_DAL.h"

/**********
 * Timers *
 **********/
void WiSafe_TimerCallback(void* handle);
void WiSafe_FaultTimerCallback(void* handle);
void WiSafe_AlarmTimerCallback(void* handle);

/**********************
 * Message queue      *
 **********************/
extern MessageQueue_t MainMessageQueue;

/*********
 * Other *
 *********/
extern EnsoErrorCode_e WiSafe_Start(void);

#endif /* _WISAFEMAIN_H_ */
