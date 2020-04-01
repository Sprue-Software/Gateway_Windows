/*!****************************************************************************
*
* \file WiSafe_DAL.h
*
* \author James Green
*
* \brief Interface between WiSafe and the Local Shadow
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFEDAL_H_
#define _WISAFEDAL_H_

#include "LSD_Types.h"
#include "APP_Types.h"
#include "WiSafe_Protocol.h"

typedef const char* propertyName_t;

#define DAL_MAXIMUM_NUMBER_WISAFE_DEVICES (50)

#define DAL_PROP_ID(x)                           (PROP_GROUP_WISAFE | x)

#define DAL_PROPERTY_IN_NETWORK_IDX               DAL_PROP_ID(1)
#define DAL_PROPERTY_UNKNOWN_SIDS_IDX             DAL_PROP_ID(2)
#define DAL_PROPERTY_DEVICE_TEST_TIMESTAMP_IDX    DAL_PROP_ID(3)
#define DAL_PROPERTY_DEVICE_SID_IDX               DAL_PROP_ID(4)
#define DAL_PROPERTY_DEVICE_MODEL_IDX             DAL_PROP_ID(5)

#define DAL_PROPERTY_DEVICE_ALARM_TEMPV_IDX       DAL_PROP_ID(6)
#define DAL_PROPERTY_DEVICE_ALARM_STATE_IDX       DAL_PROP_ID(7)
#define DAL_PROPERTY_DEVICE_BATTERY_VOLTAGE_IDX   DAL_PROP_ID(8)
#define DAL_PROPERTY_DEVICE_ALARM_SEQ_IDX         DAL_PROP_ID(9)
#define DAL_PROPERTY_DEVICE_TEMPERATURE_IDX       DAL_PROP_ID(10)
#define DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT_IDX DAL_PROP_ID(11)
#define DAL_PROPERTY_DEVICE_RADIO_RSSI_IDX        DAL_PROP_ID(12)
#define DAL_PROPERTY_DEVICE_RM_SD_FAULT_IDX       DAL_PROP_ID(13)
#define DAL_PROPERTY_DEVICE_LAST_SEQUENCE_IDX     DAL_PROP_ID(14)
#define DAL_PROPERTY_DEVICE_MUTE_IDX              DAL_PROP_ID(15)
#define DAL_COMMAND_TEST_MODE_IDX                 DAL_PROP_ID(16)
#define DAL_COMMAND_LEARN_IDX                     DAL_PROP_ID(17)
#define DAL_COMMAND_FLUSH_IDX                     DAL_PROP_ID(18)
#define DAL_COMMAND_DEVICE_FLUSH_IDX              DAL_PROP_ID(19)
#define DAL_COMMAND_SOUNDER_TEST_IDX              DAL_PROP_ID(20)
#define DAL_COMMAND_DELETE_ALL_IDX                DAL_PROP_ID(21)
#define DAL_PROPERTY_DEVICE_ALARM_TIME_IDX        DAL_PROP_ID(22)
#define DAL_COMMAND_LEARN_TIMEOUT_IDX             DAL_PROP_ID(23)
#define DAL_COMMAND_TEST_MODE_FLUSH_IDX           DAL_PROP_ID(24)
#define DAL_COMMAND_TEST_MODE_RESULT_IDX          DAL_PROP_ID(25)
#define DAL_PROPERTY_DEVICE_MISSING_IDX           DAL_PROP_ID(26)
#define DAL_PROPERTY_INTERROGATING_IDX            DAL_PROP_ID(27)
#define DAL_COMMAND_IDENTIFY_DEVICE_IDX           DAL_PROP_ID(28)
#define DAL_COMMAND_SILENCE_ALL_IDX               DAL_PROP_ID(29)
#define DAL_COMMAND_LOCATE_IDX                    DAL_PROP_ID(30)

// Property Ids 100 to 199 are reserved for fault properties and map to:
//     Property ID = 100 + the SIA code
//     Property Name = "flt<SIA code>"
// eg a fault with SIA code 7 will have property id 107 and property name "flt7"
#define DAL_PROPERTY_DEVICE_FAULT_IDX DAL_PROP_ID(100)

// Property Ids 200 to 299 are reserved for fault state properties and map to:
//     Property ID = 200 + the SIA code
//     Property Name = "flt<SIA code>_state"
#define DAL_PROPERTY_DEVICE_FAULT_STATE_IDX DAL_PROP_ID(200)

// Property Ids 300 to 399 are reserved for fault sequence properties and map to:
//     Property ID = 300 + the SIA code
//     Property Name = "flt<SIA code>_seq"
#define DAL_PROPERTY_DEVICE_FAULT_SEQ_IDX DAL_PROP_ID(300)

// Property Ids 400 to 499 are reserved for fault time properties and map to:
//     Property ID = 400 + the SIA code
//     Property Name = "flt<SIA code>_time"
#define DAL_PROPERTY_DEVICE_FAULT_TIME_IDX DAL_PROP_ID(400)

// Property Ids 500 to 599 are reserved for fault volt properties and map to:
//     Property ID = 500 + the SIA code
//     Property Name = "flt<SIA code>_battv"
#define DAL_PROPERTY_DEVICE_FAULT_BATTV_IDX DAL_PROP_ID(500)

/* Gateway properties. */
extern const propertyName_t DAL_PROPERTY_IN_NETWORK;
extern const propertyName_t DAL_PROPERTY_UNKNOWN_SIDS;

/* WiSafe device properties (read-only by platform). */
extern const propertyName_t DAL_PROPERTY_DEVICE_TEST_TIMESTAMP;
extern const propertyName_t DAL_PROPERTY_DEVICE_SID;
extern const propertyName_t DAL_PROPERTY_DEVICE_MODEL;
extern const propertyName_t DAL_PROPERTY_DEVICE_ALARM_STATE;
extern const propertyName_t DAL_PROPERTY_DEVICE_BATTERY_VOLTAGE;
extern const propertyName_t DAL_PROPERTY_DEVICE_ALARM_SEQ;
extern const propertyName_t DAL_PROPERTY_DEVICE_TEMPERATURE;
extern const propertyName_t DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT;
extern const propertyName_t DAL_PROPERTY_DEVICE_RADIO_RSSI;
extern const propertyName_t DAL_PROPERTY_DEVICE_RM_SD_FAULT;
extern const propertyName_t DAL_PROPERTY_DEVICE_LAST_SEQUENCE;
extern const propertyName_t DAL_PROPERTY_DEVICE_FAULT;
extern const propertyName_t DAL_PROPERTY_DEVICE_FAULT_STATE;
extern const propertyName_t DAL_PROPERTY_DEVICE_FAULT_SEQ;
extern const propertyName_t DAL_PROPERTY_DEVICE_FAULT_TIME;
extern const propertyName_t DAL_PROPERTY_DEVICE_FAULT_BATTV;
extern const propertyName_t DAL_PROPERTY_DEVICE_ALARM_TEMPV;
extern const propertyName_t DAL_PROPERTY_DEVICE_ALARM_TIME;
extern const propertyName_t DAL_PROPERTY_DEVICE_MISSING;
extern const propertyName_t DAL_COMMAND_DELETE_ALL;
extern const propertyName_t DAL_PROPERTY_INTERROGATING;

/* This value is used to convert between WiSafe priority values, and AWS Shadow 'type' property values. */
#define DAL_PROPERTY_DEVICE_TYPE_WISAFE_MASK 0x00010000

/* WiSafe device properties (read-writeable by platform). */
extern const propertyName_t DAL_PROPERTY_DEVICE_MUTE;

/* Gateway "command" properties (set by gateway). */
extern const propertyName_t DAL_COMMAND_TEST_MODE;
extern const propertyName_t DAL_COMMAND_TEST_MODE_FLUSH;
extern const propertyName_t DAL_COMMAND_TEST_MODE_RESULT;
extern const propertyName_t DAL_COMMAND_LEARN;
extern const propertyName_t DAL_COMMAND_LEARN_TIMEOUT;
extern const propertyName_t DAL_COMMAND_FLUSH;
extern const propertyName_t DAL_COMMAND_IDENTIFY_DEVICE;
extern const propertyName_t DAL_COMMAND_SILENCE_ALL;
extern const propertyName_t DAL_COMMAND_LOCATE;

extern const propertyName_t DAL_COMMAND_DEVICE_FLUSH;
extern const propertyName_t DAL_COMMAND_SOUNDER_TEST;

extern void WiSafe_DALInit(void);
extern void WiSafe_DALClose(void);

extern bool WiSafe_DALIsDeviceRegistered(deviceId_t id);
extern EnsoErrorCode_e WiSafe_DALRegisterDevice(deviceId_t id, uint32_t deviceType);
bool WiSafe_DALGetDeviceIdForSID(uint8_t requiredSid, deviceId_t* result);
extern void WiSafe_DALDeleteDevice(deviceId_t id);

extern EnsoErrorCode_e WiSafe_DALSetReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, EnsoValueType_e type, EnsoPropertyValue_u newValue);
extern EnsoErrorCode_e WiSafe_DALSetReportedProperties(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId[], propertyName_t name[], EnsoValueType_e type[], EnsoPropertyValue_u newValue[], int numProperties, PropertyKind_e kind,bool buffered, bool persistent);
extern EnsoPropertyValue_u WiSafe_DALGetReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoErrorCode_e* error);


extern EnsoErrorCode_e WiSafe_DALSetDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, EnsoValueType_e type, EnsoPropertyValue_u newValue);
extern EnsoPropertyValue_u WiSafe_DALGetDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoErrorCode_e* error);

extern EnsoErrorCode_e WiSafe_DALSubscribeToDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t pid);

extern EnsoErrorCode_e WiSafe_DALDevicesEnumerate(deviceId_t* buffer, uint16_t size, uint16_t* numDevices);

extern EnsoErrorCode_e WiSafe_DALIncrementReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name);
extern EnsoDeviceId_t EnsoDeviceFromWiSafeID(deviceId_t id);
extern EnsoErrorCode_e WiSafe_DALCreateProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, EnsoValueType_e type, EnsoPropertyValue_u newValue);
extern EnsoErrorCode_e WiSafe_DALDeleteProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name);

#endif /* _WISAFEDAL_H_ */
