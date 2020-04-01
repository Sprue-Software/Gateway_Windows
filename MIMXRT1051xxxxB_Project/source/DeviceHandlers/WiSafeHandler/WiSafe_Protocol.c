/*!****************************************************************************
*
* \file WiSafe_Protocol.cpp
*
* \author James Green
*
* \brief Format/pack messages for transmission over SPI.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stddef.h>
#include <assert.h>
#include <string.h>

#include "Sprue/spruerm_protocol.h"

#include "WiSafe_Protocol.h"

enum spruediagvariableindex {
    diag_variable_sd_voltage  = 0x06,
    diag_variable_temperature = 0x06 // both 6
};

enum sprueSounder {
    sprueSounderEnabled  = 0x01,
    sprueSounderDisabled = 0x02
};

enum sprueopt {
    // EXT DIAG options
    missing_node       = 0x01,
    sid_map            = 0x03,
    sid_map_upd        = 0x04,
    ext_diag_undef     = 0x05,
    remote_rm_status   = 0x06,
    remote_id_report   = 0x09,
    neighbour_check    = 0x10,
    remote_map         = 0x11,
    ext_learn_in_press = 0x12,
    set_rm_config      = 0x14,
    config_req         = 0x15,
    initial_crc        = 0x16,
    lurk_map           = 0x17,
    disable_lurk       = 0x18,
    rumour_target      = 0x1a,

    // RM remote production test respon(c)e options
    spi_comms_check    = 0x01,
    low_battery_set    = 0x03,
    rf_conn_test       = 0x07,

    // Request type
    request_remote_status = 0x00,
    request_id_details    = 0x01
};

uint8_t  gateWaySid = 64; //ABR added

/**
 * Construct a new "ident" alarm and initialise with default values.
 *
 * @return The alarm message.
 */
msgAlarmIdent_t WiSafe_NewAlarmIdent(void)
{
    msgAlarmIdent_t result;

    result.id = 0; // The previous implementation did this, so we do the same.
    result.model = GATEWAY_DEVICE_MODEL;
    result.priority = GATEWAY_DEVICE_PRIORITY;
    result.firmwareMajor = FIRMWARE_VERSION_MAJOR;
    result.firmwareMinor = FIRMWARE_VERSION_MINOR;

    result.batteryFaultRM = false;
    result.acFailed = false;
    result.batteryFaultSD = false;
    result.isOnBase = false;
    result.faulty = false;
    result.calibrated = false;

    return result;
}

/**
 * Pack an "ident" alarm into a comms buffer.
 *
 * @param alarmIdent
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeAlarmIdent(msgAlarmIdent_t* alarmIdent)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 10);
        result->count = 10;

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_ALARM_IDENT);
        result->data[1] = ((alarmIdent->id >> 0)  & 0xff);
        result->data[2] = ((alarmIdent->id >> 8)  & 0xff);
        result->data[3] = ((alarmIdent->id >> 16) & 0xff);
        result->data[4] = ((alarmIdent->model >> 0) & 0xff);
        result->data[5] = ((alarmIdent->model >> 8) & 0xff);
        result->data[6] = alarmIdent->priority;
        result->data[7] = (alarmIdent->batteryFaultRM?(1 << 5):0) |
                                (alarmIdent->acFailed?(1 << 4):0) |
                          (alarmIdent->batteryFaultSD?(1 << 3):0) |
                                (alarmIdent->isOnBase?(1 << 2):0) |
                                  (alarmIdent->faulty?(1 << 1):0) |
                              (alarmIdent->calibrated?(1 << 0):0);
        result->data[8] = alarmIdent->firmwareMinor;
        result->data[9] = alarmIdent->firmwareMajor;
    }

    return result;
}

/**
 * Pack a 'request SID map' message into a comms buffer.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeRequestSIDMap(void)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 2);
        result->data[0] = SPRUERM_CMD_OF(SPRUERM_RM_EXTDIAG_REQUEST);
        result->data[1] = sprueOptSidMap;
        result->count = 2;
    }

    return result;
}

/**
 * Unpack a 'device tested' message from the given buffer.
 *
 * @param buffer The buffer holding the message recevied over SPI.
 *
 * @return The unpacked structure.
 */
msgDeviceTested_t WiSafe_DecodeDeviceTested(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 10);

    msgDeviceTested_t result;
    uint8_t* data = &(buffer->data[0]);
    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.priority = data[4];
    result.status = data[5];
    result.model = (data[6] | (data[7] << 8));
    result.sid = data[8];
    result.seq = data[9];

    return result;
}

/**
 * Unpack a 'Request Remote ID Results' message from the given buffer.
 *
 * @param buffer The buffer holding the message received over SPI.
 *
 * @return The unpacked structure.
 */
msgRequestRemoteIdResult_t WiSafe_DecodeRemoteIdResult(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 9);

    msgRequestRemoteIdResult_t result;
    uint8_t* data = &(buffer->data[0]);
    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.priority = data[4];
    result.model = (data[5] | (data[6] << 8));
    result.sid = data[7];
    result.seq = data[8];

    return result;
}

/**
 * Unpack a 'fault' message from the given buffer.
 *
 * @param buffer The buffer holding the message recevied over SPI.
 *
 * @return The unpacked structure.
 */
msgFault_t WiSafe_DecodeFault(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 9);

    msgFault_t result;
    uint8_t* data = &(buffer->data[0]);
    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.model = (data[4] | (data[5] << 8));
    result.flags = data[6];
    result.sid = data[7];
    result.seq = data[8];

    return result;
}

/**
 * Unpack an 'alarm' message from the given buffer.
 *
 * @param buffer The buffer holding the message recevied over SPI.
 *
 * @return The unpacked structure.
 */
msgAlarm_t WiSafe_DecodeAlarm(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 8);

    msgAlarm_t result;
    uint8_t* data = &(buffer->data[0]);
    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.priority = data[4];
    result.status = data[5];
    result.sid = data[6];
    result.seq = data[7];

    return result;
}

/**
 * Pack an 'update SID map' message into a comms buffer using the map
 * provided.
 *
 * @param map The SID map to be encoded.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeUpdateSIDMap(uint8_t* map)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 10);
        result->data[0] = SPRUERM_CMD_OF(SPRUERM_RM_EXTDIAG_REQUEST);
        result->data[1] = sprueOptidMapUpd;
        memcpy(&(result->data[2]), map, 8);
        result->count = 10;
    }

    return result;
}

/**
 * Pack an 'RM Learn-in' message into a comms buffer.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeRMLearnIn(void)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 3);
        result->data[0] = SPRUERM_CMD_OF(SPRUERM_RM_EXTDIAG_REQUEST);
        result->data[1] = sprueOptExtLearnInPress;
        result->data[2] = 0x01; // Number of presses.
        result->count = 3;
    }

    return result;
}

/**
 * Pack a 'Read Voltage' message into a comms buffer.
 *
 * @param id The ID of the device to receive the message.
 * @param model The model of the device.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeReadVolts(deviceId_t id, deviceModel_t model)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 7);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_DO_EXTDIG_DATASLOT);

        result->data[1] = ((id >> 0)  & 0xff);
        result->data[2] = ((id >> 8)  & 0xff);
        result->data[3] = ((id >> 16) & 0xff);

        result->data[4] = ((model >> 0) & 0xff);
        result->data[5] = ((model >> 8) & 0xff);

        result->data[6] = diag_variable_sd_voltage;

        result->count = 7;
    }

    return result;
}

/**
 * Pack a 'Read Temperature' message into a comms buffer.
 *
 * @param id The ID of the device to receive the message.
 * @param model The model of the device.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeReadTemp(deviceId_t id, deviceModel_t model)
{
    /* The message format is exactly the same as the read volts. */
    return WiSafe_EncodeReadVolts(id, model);
}

/**
 * Pack a 'Request Fault Details' message into a comms buffer.
 *
 * @param id The ID of the device to receive the message.
 * @param model The model of the device.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeRequestFaultDetails(deviceId_t id, deviceModel_t model)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 7);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_REQ_FAULT_DETAILS);

        result->data[1] = ((id >> 0)  & 0xff);
        result->data[2] = ((id >> 8)  & 0xff);
        result->data[3] = ((id >> 16) & 0xff);

        result->data[4] = ((model >> 0) & 0xff);
        result->data[5] = ((model >> 8) & 0xff);

        result->data[6] = 0x00;

        result->count = 7;
    }

    return result;
}

/**
 * Unpack a 'Extended Diagnostic Variable Response' message from the given buffer.
 *
 * @param buffer The buffer holding the message received over SPI.
 *
 * @return The unpacked structure.
 */
msgExtDiagVarResp_t WiSafe_DecodeExtDiagVarResp(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 11);

    msgExtDiagVarResp_t result;
    uint8_t* data = &(buffer->data[0]);

    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.slot = data[4];
    result.variable = ((uint16_t)data[5] | ((uint16_t)data[6] << 8));
    result.sid = data[9];
    result.seq = data[10];

    return result;
}

/**
 * Unpack a 'fault detail' message from the given buffer.
 *
 * @param buffer The buffer holding the message recevied over SPI.
 *
 * @return The unpacked structure.
 */
msgFaultDetail_t WiSafe_DecodeFaultDetail(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 10);

    msgFaultDetail_t result;
    uint8_t* data = &(buffer->data[0]);

    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.model = (data[4] | (data[5] << 8));
    result.faults = ((uint16_t)data[6] | ((uint16_t)data[7] << 8));
    result.sid = data[8];
    result.seq = data[9];

    return result;
}

/**
 * Unpack an 'alarm stop' message from the given buffer.
 *
 * @param buffer The buffer holding the message recevied over SPI.
 *
 * @return The unpacked structure.
 */
msgAlarmStop_t WiSafe_DecodeAlarmStop(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 7);

    msgAlarmStop_t result;
    uint8_t* data = &(buffer->data[0]);

    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.priority = data[4];
    result.sid = data[5];
    result.seq = data[6];

    return result;
}

/**
 * Unpack a 'hush' message from the given buffer.
 *
 * @param buffer The buffer holding the message received over SPI.
 *
 * @return The unpacked structure.
 */
msgHush_t WiSafe_DecodeHush(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 7);

    msgHush_t result;
    uint8_t* data = &(buffer->data[0]);

    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.priority = data[4];
    result.sid = data[5];
    result.seq = data[6];

    return result;
}

/**
 * Unpack a 'locate' message from the given buffer.
 *
 * @param buffer The buffer holding the message received over SPI.
 *
 * @return The unpacked structure.
 */
msgLocate_t WiSafe_DecodeLocate(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 8);

    msgLocate_t result;
    uint8_t* data = &(buffer->data[0]);

    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.priority = data[4];
    result.originating = data[5];
    result.sid = data[6];
    result.seq = data[7];

    return result;
}

/**
 * Pack a 'Device Test' message into a comms buffer.
 *
 * @param id The ID of the device to receive the message.
 * @param model The model of the device.
 * @param priority The priority of the device.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeDeviceTest(deviceId_t id, deviceModel_t model, devicePriority_t priority)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 8);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_DEVICE_TESTED);

        result->data[1] = ((id >> 0)  & 0xff);
        result->data[2] = ((id >> 8)  & 0xff);
        result->data[3] = ((id >> 16) & 0xff);

        result->data[4] = priority;

        result->data[5] = 0x01;

        result->data[6] = ((model >> 0) & 0xff);
        result->data[7] = ((model >> 8) & 0xff);

        result->count = 8;
    }

    return result;
}

/**
 * Pack a 'Sounder Enable' message into a comms buffer.
 *
 * @param id The ID of the device to receive the message.
 * @param enabled Whether to enable or disable the sounder.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeSounderEnable(deviceId_t id, bool enabled)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 9);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_NOTIFY_DEV_ID);

        result->data[1] = ((id >> 0)  & 0xff);
        result->data[2] = ((id >> 8)  & 0xff);
        result->data[3] = ((id >> 16) & 0xff);

        result->data[4] = enabled?sprueSounderEnabled:sprueSounderDisabled;

        result->data[5] = 0;
        result->data[6] = 0;
        result->data[7] = 0;
        result->data[8] = 0;

        result->count = 9;
    }

    return result;
}

/**
 * Unpack a 'RM diagnostic' message from the given buffer.
 *
 * @param buffer The buffer holding the message recevied over SPI.
 *
 * @return The unpacked structure.
 */
msgRMDiagnosticResult_t WiSafe_DecodeRMDiagnosticResult(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 13);

    msgRMDiagnosticResult_t result;
    uint8_t* data = &(buffer->data[0]);

    result.vBatNoLoad = data[1];
    result.vBatWithLoad = data[2];
    result.vBat2ndSupply = data[3];
    result.lastRSSI = data[4];
    result.firmwareRevision = data[5];
    result.id = (data[6] | (data[7] << 8) | (data[8] << 16));
    result.criticalFlags = data[9];
    result.radioFaultCount = data[10];
    result.sid = data[11];

    gateWaySid = data[11]; //ABR added

    return result;
}

/**
 * Unpack an 'RM SD Fault' message from the given buffer.
 *
 * @param buffer The buffer holding the message recevied over SPI.
 *
 * @return The unpacked structure.
 */
msgRMSDFault_t WiSafe_DecodeRMSDFault(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 7);

    msgRMSDFault_t result;
    uint8_t* data = &(buffer->data[0]);

    result.id = (data[1] | (data[2] << 8) | (data[3] << 16));
    result.model = (data[4] | (data[5] << 8));
    result.sid = data[6];

    return result;
}

/**
 * Pack a 'Request Remote Status' message into a comms buffer.
 *
 * @param sid The SID of the device to receive the message.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeRequestRemoteStatus(sid_t sid)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 4);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_RM_EXTDIAG_REQUEST);
        result->data[1] = remote_rm_status;
        result->data[2] = sid;
        result->data[3] = request_remote_status;

        result->count = 4;
    }

    return result;
}

/**
 * Pack a 'Request Remote ID Details' message into a comms buffer.
 *
 * @param sid The SID of the device to receive the message.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeRequestRemoteIDDetails(sid_t sid)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 4);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_RM_EXTDIAG_REQUEST);
        result->data[1] = remote_rm_status;
        result->data[2] = sid;
        result->data[3] = request_id_details;

        result->count = 4;
    }

    return result;
}

/**
 * Pack a 'Rumour Target' message into a comms buffer.
 *
 * @param sid The SID of the device to receive the message.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeRumourTarget(sid_t sid)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 3);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_RM_EXTDIAG_REQUEST);
        result->data[1] = rumour_target;
        result->data[2] = sid;

        result->count = 3;
    }

    return result;
}

/**
 * Pack a 'RM Doagnostic Request' message into a comms buffer.
 *
 * @param sid The SID of the device to receive the message.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeRMDiagnosticRequest(void)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 1);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_RM_DIAGNOSTIC_REQUEST);

        result->count = 1;
    }

    return result;
}

/**
 * Pack a 'Hush' message into a comms buffer.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeHush(void)
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 5);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_HUSH);
        // The next 3 bytes are the device id which is zero for a gateway
        result->data[1] = 0;
        result->data[2] = 0;
        result->data[3] = 0;
        result->data[4] = GATEWAY_DEVICE_PRIORITY;

        result->count = 5;
    }

    return result;
}

/**
 * Pack a 'Locate' message into a comms buffer.
 *
 * @return The buffer, or NULL in no free buffers.
 */
radioCommsBuffer_t* WiSafe_EncodeLocate()
{
    radioCommsBuffer_t* result = WiSafe_RadioCommsBufferGet();

    if (result != NULL)
    {
        assert(WiSafe_RadioCommsBufferRemainingSpace(result) >= 6);

        result->data[0] = SPRUERM_CMD_OF(SPRUERM_LOCATE);

        // Set the gateway's device id and priority
        result->data[1] = 0;
        result->data[2] = 0;
        result->data[3] = 0;

        result->data[4] = GATEWAY_DEVICE_PRIORITY;

        // Set Originating to zero as the gateway is not in alarm state.
        result->data[5] = 0;

        result->count = 6;
    }

    return result;
}

/**
 * Unpack a 'Remote Status Report' message from the given buffer.
 *
 * @param buffer The buffer holding the message recevied over SPI.
 *
 * @return The unpacked structure.
 */
msgRemoteStatusResult_t WiSafe_DecodeRemoteStatusReport(radioCommsBuffer_t* buffer)
{
    assert(buffer->count == 13);

    msgRemoteStatusResult_t result;
    uint8_t* data = &(buffer->data[0]);

    result.sid = data[2];
    result.vBatNoLoad = data[3];
    result.vBatWithLoad = data[4];
    result.vBat2ndSupply = data[5];
    result.lastRSSI = data[6];
    result.firmwareRevision = data[7];
    result.id = (data[8] | (data[9] << 8) | (data[10] << 16));
    result.criticalFaults = data[11];
    result.radioFaultCount = data[12];

    return result;
}
