/*!****************************************************************************
*
* \file WiSafe_Control.h
*
* \author James Green
*
* \brief Interface allowing control of the WiSafe Device Handler.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFECONTROL_H_
#define _WISAFECONTROL_H_

#include <stdint.h>

#include "WiSafe_Protocol.h"

extern void WiSafe_ControlBeginMissingNodeScan(bool deleteMissingDevices);
extern void WiSafe_ControlStopMissingNodeScan(void);

extern void WiSafe_ControlLearnMode(bool enabled, uint32_t timeout);

extern void WiSafe_ControlIssueTest(deviceId_t id);

extern void WiSafe_ControlFlushAll(void);
extern void WiSafe_ControlFlush(deviceId_t id);
extern void WiSafe_ControlLeave(void);
extern void WiSafe_ControlIdentifyDevice(deviceId_t id);
extern void WiSafe_ControlSilenceAll(void);
extern void WiSafe_ControlLocate(void);

typedef enum
{
    controlOp_beginMissingNodeScan,
    controlOp_stopMissingNodeScan,
    controlOp_issueTest,

    controlOp_learn,

    controlOp_flushAll,
    controlOp_flush,
    controlOp_leave,
    controlOp_identifyDevice,
    controlOp_silenceAll,
    controlOp_locate,

} controlOp_e;

typedef struct _controlEntry_t
{
    controlOp_e operation;
    uint32_t    param1;
    uint32_t    param2;
} controlEntry_t;

typedef struct
{
    uint8_t messageType;
    controlEntry_t* control;
} commandMessage_st;

#endif /* _WISAFECONTROL_H_ */
