/*!****************************************************************************
*
* \file WiSafe_Protocol.h
*
* \author James Green
*
* \brief Format/pack messages for transmission over SPI.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFEPROTOCOL_H_
#define _WISAFEPROTOCOL_H_

#include <stdint.h>

#include "WiSafe_RadioCommsBuffer.h"

#define FIRMWARE_VERSION_MAJOR 0x09
#define FIRMWARE_VERSION_MINOR 0x0a

#define GATEWAY_DEVICE_MODEL 0x0506

#define GATEWAY_DEVICE_PRIORITY 0xc9

#define GATEWAY_DEVICE_ID 0xffffff

// G2-556 Can we remove this type and use EnsoDeviceId_t?
typedef uint32_t deviceId_t;

typedef uint16_t deviceModel_t;

typedef uint8_t devicePriority_t;

typedef uint8_t sid_t;

typedef uint8_t seq_t;

typedef struct
{
    deviceId_t id;
    deviceModel_t model;
    devicePriority_t priority;
    uint8_t firmwareMajor;
    uint8_t firmwareMinor;

    bool batteryFaultRM;
    bool acFailed;
    bool batteryFaultSD;
    bool isOnBase;
    bool faulty;
    bool calibrated;
} msgAlarmIdent_t;

typedef struct
{
    deviceId_t id;
    devicePriority_t priority;
    uint8_t status;
    deviceModel_t model;
    sid_t sid;
    seq_t seq;
} msgDeviceTested_t;

typedef struct
{
    deviceId_t id;
    deviceModel_t model;
    uint8_t flags;
    sid_t sid;
    seq_t seq;
} msgFault_t;

typedef struct
{
    deviceId_t id;
    devicePriority_t priority;
    uint8_t status;
    sid_t sid;
    seq_t seq;
} msgAlarm_t;

typedef struct
{
    deviceId_t id;
    uint8_t slot;
    uint16_t variable;
    sid_t sid;
    seq_t seq;
} msgExtDiagVarResp_t;

typedef struct
{
    deviceId_t id;
    deviceModel_t model;
    uint16_t faults;
    sid_t sid;
    seq_t seq;
} msgFaultDetail_t;

typedef struct
{
    deviceId_t id;
    devicePriority_t priority;
    sid_t sid;
    seq_t seq;
} msgAlarmStop_t;

typedef struct
{
    deviceId_t id;
    devicePriority_t priority;
    sid_t sid;
    seq_t seq;
} msgHush_t;

typedef struct
{
    deviceId_t id;
    devicePriority_t priority;
    uint8_t originating;
    sid_t sid;
    seq_t seq;
} msgLocate_t;

typedef struct
{
    deviceId_t id;
    uint8_t vBatNoLoad;
    uint8_t vBatWithLoad;
    uint8_t vBat2ndSupply;
    uint8_t lastRSSI;
    uint8_t firmwareRevision;
    uint8_t criticalFlags;
    uint8_t radioFaultCount;
    sid_t sid;
} msgRMDiagnosticResult_t;

typedef struct
{
    sid_t sid;
    uint8_t vBatNoLoad;
    uint8_t vBatWithLoad;
    uint8_t vBat2ndSupply;
    uint8_t lastRSSI;
    uint8_t firmwareRevision;
    deviceId_t id;
    uint8_t criticalFaults;
    uint8_t radioFaultCount;
} msgRemoteStatusResult_t;

typedef struct
{
    deviceId_t id;
    deviceModel_t model;
    sid_t sid;
} msgRMSDFault_t;

typedef struct
{
    deviceId_t id;
    devicePriority_t priority;
    deviceModel_t model;
    sid_t sid;
    seq_t seq;
} msgRequestRemoteIdResult_t;

typedef enum {
    /* EXT DIAG options. */
    sprueOptMissingNode     = 0x01,
    sprueOptSidMap          = 0x03,
    sprueOptidMapUpd        = 0x04,
    sprueOptExtDiagUndef    = 0x05,
    sprueOptRemoteRmStatus  = 0x06,
    sprueOptRemoteIdReport  = 0x09,
    sprueOptNeighbourCheck  = 0x10,
    sprueOptRemoteMap       = 0x11,
    sprueOptExtLearnInPress = 0x12,
    sprueOptSetRmConfig     = 0x14,
    sprueOptConfigReq       = 0x15,
    sprueOptInitialCrc      = 0x16,
    sprueOptLurkMap         = 0x17,
    sprueOptDisableLurk     = 0x18,
    sprueOptRumourTarget    = 0x1a,

    /* RM remote production test respon(c)e options. */
    sprueOptSpiCommsCheck   = 0x01,
    sprueOptLowBatterySet   = 0x03,
    sprueOptRfConnTest      = 0x07,

    // Request type
    sprueOptRequestRemoteStatus = 0x00,
    sprueOptRequestIdDetails    = 0x01
} sprueOpt_e;

extern uint8_t  gateWaySid; //ABR added

extern msgAlarmIdent_t WiSafe_NewAlarmIdent(void);
extern radioCommsBuffer_t* WiSafe_EncodeAlarmIdent(msgAlarmIdent_t* alarmIdent);
extern radioCommsBuffer_t* WiSafe_EncodeRequestSIDMap(void);
extern radioCommsBuffer_t* WiSafe_EncodeRMDiagnosticRequest(void);

extern msgDeviceTested_t WiSafe_DecodeDeviceTested(radioCommsBuffer_t* buffer);

extern msgRequestRemoteIdResult_t WiSafe_DecodeRemoteIdResult(radioCommsBuffer_t* buffer);

extern msgFault_t WiSafe_DecodeFault(radioCommsBuffer_t* buffer);

extern msgAlarm_t WiSafe_DecodeAlarm(radioCommsBuffer_t* buffer);

extern radioCommsBuffer_t* WiSafe_EncodeUpdateSIDMap(uint8_t* map);

extern radioCommsBuffer_t* WiSafe_EncodeRMLearnIn(void);

extern radioCommsBuffer_t* WiSafe_EncodeReadVolts(deviceId_t id, deviceModel_t model);

extern radioCommsBuffer_t* WiSafe_EncodeReadTemp(deviceId_t id, deviceModel_t model);

extern radioCommsBuffer_t* WiSafe_EncodeRequestFaultDetails(deviceId_t id, deviceModel_t model);

extern msgExtDiagVarResp_t WiSafe_DecodeExtDiagVarResp(radioCommsBuffer_t* buffer);

extern msgFaultDetail_t WiSafe_DecodeFaultDetail(radioCommsBuffer_t* buffer);

extern msgAlarmStop_t WiSafe_DecodeAlarmStop(radioCommsBuffer_t* buffer);

extern msgHush_t WiSafe_DecodeHush(radioCommsBuffer_t* buffer);

extern msgLocate_t WiSafe_DecodeLocate(radioCommsBuffer_t* buffer);

extern radioCommsBuffer_t* WiSafe_EncodeDeviceTest(deviceId_t id, deviceModel_t model, devicePriority_t priority);

extern radioCommsBuffer_t* WiSafe_EncodeSounderEnable(deviceId_t id, bool enabled);

extern radioCommsBuffer_t* WiSafe_EncodeHush(void);

extern radioCommsBuffer_t* WiSafe_EncodeLocate();

extern msgRMDiagnosticResult_t WiSafe_DecodeRMDiagnosticResult(radioCommsBuffer_t* buffer);

extern msgRMSDFault_t WiSafe_DecodeRMSDFault(radioCommsBuffer_t* buffer);

extern radioCommsBuffer_t* WiSafe_EncodeRequestRemoteStatus(sid_t sid);

extern radioCommsBuffer_t* WiSafe_EncodeRequestRemoteIDDetails(sid_t sid);

extern msgRemoteStatusResult_t WiSafe_DecodeRemoteStatusReport(radioCommsBuffer_t* buffer);

extern radioCommsBuffer_t* WiSafe_EncodeRumourTarget(sid_t sid);

#endif /* _WISAFEPROTOCOL_H_ */
