/*!****************************************************************************
*
* \file WiSafe_Discovery.h
*
* \author James Green
*
* \brief Handle discovery of other WiSafe devices.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFEDISCOVERY_H_
#define _WISAFEDISCOVERY_H_

#include "WiSafe_Timer.h"
#include "WiSafe_RadioCommsBuffer.h"
#include "WiSafe_DAL.h"

/* Constant definitions. */
extern const char* WISAFE_PROFILE_KEY_NAME;
extern const char* WISAFE_PROFILE_KEY_READ_VOLTS_ON_FAULT;
extern const char* WISAFE_PROFILE_KEY_FAULT_DETAILS_TYPE;
extern const char* WISAFE_PROFILE_KEY_TEMPERATURE_ALARM;

/* Online Status enumeration to correspond to G2-7. */
typedef enum {
    ONLINE_STATUS_OFFLINE = 0,
    ONLINE_STATUS_ONLINE  = 1,
    ONLINE_STATUS_UNKNOWN = 2
} onlineStatus_e;

extern void WiSafe_DiscoveryInit(void);

extern void WiSafe_DiscoveryHandleTimer(timerType_e type);
extern void WiSafe_DiscoveryProcessSIDMap(uint8_t* mapData);
extern void WiSafe_DiscoveryProcessDeviceTested(msgDeviceTested_t* msg,
                                                onlineStatus_e onlineStatus, bool realTestMsg);/*ABR changed*/
extern void WiSafe_DiscoveryMissingNodeReport(uint8_t* mapData);

extern void WiSafe_DiscoveryLearnMode(bool enabled, uint32_t timeout);
extern void WiSafe_DiscoveryTestMode(bool enabled, bool deleteMissingNodes);

extern char* WiSafe_DiscoveryFindProfile(uint32_t deviceType);
extern bool WiSafe_DiscoveryIsFeatureSupported(deviceId_t id, const char* key);
extern int32_t WiSafe_DiscoveryFaultDetailsType(deviceId_t id);

extern deviceModel_t WiSafe_DiscoveryGetModelForDevice(deviceId_t id);

extern void WiSafe_DiscoveryLeave(void);

extern void WiSafe_DiscoveryFlushAll(void);
extern void WiSafe_DiscoveryFlush(deviceId_t id);

extern void WiSafe_DiscoveryProcessSIDMapUpdate(uint8_t* mapData);
extern void WiSafe_DiscoveryProcessRumourTargetResponse(void);

extern void WiSafe_DiscoveryRemoteStatusReceived(msgRequestRemoteIdResult_t* msg);

extern uint32_t testMissingNormalPeriod;

#endif /* _WISAFEDISCOVERY_H_ */
