/*!****************************************************************************
*
* \file WiSafe_Constants.c
*
* \author Gavin Dolling
*
* \brief Constants that are shared betweeen handler and tests
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*/

#include "WiSafe_DAL.h"


/* These definitions are here (and extern) so that we can guarantee
 * only only copy of them in memory, to ensure maximum efficiency. */
const propertyName_t DAL_PROPERTY_IN_NETWORK                  = (propertyName_t)"netwk";
const propertyName_t DAL_PROPERTY_UNKNOWN_SIDS                = (propertyName_t)"unkn";
const propertyName_t DAL_PROPERTY_DEVICE_TEST_TIMESTAMP       = (propertyName_t)"testb";
const propertyName_t DAL_PROPERTY_DEVICE_SID                  = (propertyName_t)"sid";
const propertyName_t DAL_PROPERTY_DEVICE_MODEL                = (propertyName_t)"mdVal"; // Numeric representation of model.
const propertyName_t DAL_PROPERTY_DEVICE_ALARM_TEMPV          = (propertyName_t)"alarm_tempv";
const propertyName_t DAL_PROPERTY_DEVICE_ALARM_STATE          = (propertyName_t)"alarm_state";
const propertyName_t DAL_PROPERTY_DEVICE_BATTERY_VOLTAGE      = (propertyName_t)"battv";
const propertyName_t DAL_PROPERTY_DEVICE_ALARM_SEQ            = (propertyName_t)"alarm_seq";
const propertyName_t DAL_PROPERTY_DEVICE_TEMPERATURE          = (propertyName_t)"tempv";
const propertyName_t DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT    = (propertyName_t)"rdFlt";
const propertyName_t DAL_PROPERTY_DEVICE_RADIO_RSSI           = (propertyName_t)"rssi";
const propertyName_t DAL_PROPERTY_DEVICE_RM_SD_FAULT          = (propertyName_t)"rmsdf";
const propertyName_t DAL_PROPERTY_DEVICE_LAST_SEQUENCE        = (propertyName_t)"ltseq";
const propertyName_t DAL_COMMAND_DELETE_ALL                   = (propertyName_t)"rmall_trgrd";
const propertyName_t DAL_PROPERTY_INTERROGATING               = (propertyName_t)"dvint";

const propertyName_t DAL_PROPERTY_DEVICE_ALARM_TIME           = (propertyName_t)"alarm_time";
const propertyName_t DAL_PROPERTY_DEVICE_MISSING              = (propertyName_t)"dvmis";


// Property Ids 100 to 199 are reserved for fault properties and map to:
//     Property ID = 100 + the SIA code
//     Property Name = "flt<SIA code>"
// eg a fault with SIA code 93 will have property id 193 and property name "flt93"
const propertyName_t DAL_PROPERTY_DEVICE_FAULT                = (propertyName_t)"flt";

// Property Ids 200 to 299 are reserved for fault state properties and map to:
//     Property ID = 200 + the SIA code
//     Property Name = "flt<SIA code>_state"
//     Example:        "flt93_state"
const propertyName_t DAL_PROPERTY_DEVICE_FAULT_STATE          = (propertyName_t)"_state";

// Property Ids 300 to 399 are reserved for fault sequence properties and map to:
//     Property ID = 300 + the SIA code
//     Property Name = "flt<SIA code>_seq"
//     Example:        "flt93_seq"
const propertyName_t DAL_PROPERTY_DEVICE_FAULT_SEQ            = (propertyName_t)"_seq";

// Property Ids 400 to 499 are reserved for fault time properties and map to:
//     Property ID = 400 + the SIA code
//     Property Name = "flt<SIA code>_time"
//     Example:        "flt93_time"
const propertyName_t DAL_PROPERTY_DEVICE_FAULT_TIME           = (propertyName_t)"_time";

// Property Ids 500 to 599 are reserved for fault volt properties and map to:
//     Property ID = 500 + the SIA code
//     Property Name = "flt<SIA code>_battv"
//     Example:        "flt93_battv"
const propertyName_t DAL_PROPERTY_DEVICE_FAULT_BATTV          = (propertyName_t)"_battv";

const propertyName_t DAL_PROPERTY_DEVICE_MUTE                 = (propertyName_t)"mute";

const propertyName_t DAL_COMMAND_TEST_MODE                    = (propertyName_t)"tstmd_trgrd";
const propertyName_t DAL_COMMAND_TEST_MODE_FLUSH              = (propertyName_t)"tstmd_flush";
const propertyName_t DAL_COMMAND_TEST_MODE_RESULT             = (propertyName_t)"tstmd_reslt";

const propertyName_t DAL_COMMAND_LEARN                        = (propertyName_t)"learn_trgrd";
const propertyName_t DAL_COMMAND_LEARN_TIMEOUT                = (propertyName_t)"learn_tmout";

const propertyName_t DAL_COMMAND_SOUNDER_TEST                 = (propertyName_t)"sntst_trgrd";

const propertyName_t DAL_COMMAND_IDENTIFY_DEVICE              = (propertyName_t)"iddev_trgrd";

const propertyName_t DAL_COMMAND_LOCATE                       = (propertyName_t)"lcdev_trgrd";

const propertyName_t DAL_COMMAND_SILENCE_ALL                  = (propertyName_t)"slall_trgrd";

const propertyName_t DAL_COMMAND_FLUSH                        = (propertyName_t)"flush_trgrd";

const propertyName_t DAL_COMMAND_DEVICE_FLUSH                 = (propertyName_t)"c_fld";
