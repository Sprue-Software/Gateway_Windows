/*!****************************************************************************
 *
 * \file SYS_Gateway.c
 *
 * \author Evelyne Donnaes
 *
 * \brief Gateway Device Implementation
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "SYS_Gateway.h"
#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "LSD_Api.h"
#include "LOG_Api.h"
#include "HAL.h"
#include "APP_Types.h"
#include "KVP_Api.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
/*!****************************************************************************
 * Constants
 *****************************************************************************/

/** Gateway device constants */
#define GW_DEVICE_TYPE         0xffff


// This is normally passed in by -D flags to the compiler in the Build Environment.
#ifndef GW_FWVER
#define GW_FWVER  "0.1"
#endif

// This is normally passed in by -D flags to the compiler in the Build Environment.
#ifndef GW_FWTIM
#define GW_FWTIM  "0"
#endif

// Default firmware name if not given at build time or cannot be read from the OS.
#define GW_FWNAMDEFAULT  "099_sprue_no_upgrade_sprue_dev"

#define FW_VERSION_PATH "/etc/fwversion.json"
#define FW_VERSION_JSON_LEN 300

/*
 * Demo Properties
 */
#define PROP_DEMO_LTMOUT_ID                     (PROP_GROUP_GATEWAY | 0x0030)
#define PROP_DEMO_LTRGRD_ID                     (PROP_GROUP_GATEWAY | 0x0031)
#define PROP_DEMO_UNKN_ID                       (PROP_GROUP_GATEWAY | 0x0032)
#define PROP_DEMO_TESTMODE_ID                   (PROP_GROUP_GATEWAY | 0x0033)
#define PROP_DEMO_TESTRES_ID                    (PROP_GROUP_GATEWAY | 0x0034)

/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

static EnsoDeviceId_t _deviceId;
static EnsoObject_t* _sysObject;

// FWNAM may be passed in by -D flags to the compiler in the Build Environment.
// The fw name will be in the format:
//     <distribution index>_<hardware version>_<major>_<minor>_<name>-<version>
// Example:
//     1_sprue_1_36_sprue_hub_intamac_sprue_dev-2.0
// Declare a buffer big enough for the fw name property
#define FW_HWVER_LEN 10
#define FW_NAME_LEN 100
#define FW_DI_LEN 10
#define FW_MAJOR_LEN 10
#define FW_MINOR_LEN 10
#define FW_VERSION_LEN 5
#define MAX_FWNAME_LENGTH (FW_DI_LEN + FW_HWVER_LEN + FW_MAJOR_LEN + FW_MINOR_LEN + FW_NAME_LEN + FW_VERSION_LEN + 6)
#ifdef GW_FWNAM
// Set firmware name to the build value
char gw_fwnam[MAX_FWNAME_LENGTH + 1] = GW_FWNAM;
#else
// Read the firmware name from the operating system later
char gw_fwnam[MAX_FWNAME_LENGTH + 1] = {0};
#endif

/******************************************************************************
 * Private Helper Functions
 *****************************************************************************/

typedef struct
{
    EnsoAgentSidePropertyId_t id;
    char name[LSD_PROPERTY_NAME_BUFFER_SIZE];
    EnsoPropertyType_t type;
    uint32_t kind;
    EnsoPropertyValue_u val[PROPERTY_GROUP_MAX];
    PropertyGroup_e group;
    HandlerId_e handler;
} Prop_Init_t;

#define GW_FWNAM "SA2941_R_1_03" //nishi
/**
 * \name SYS_CreateProperty
 *
 * \brief Helper function to create a property

 * \param  p     pointer to initialisation strcuture
 *
 * \return EnsoErrorCode_e
 */
static EnsoErrorCode_e SYS_CreateProperty(const Prop_Init_t * p)
{
    EnsoErrorCode_e result = LSD_CreateProperty(_sysObject, p->id, p->name,
        p->type.valueType, p->type.kind, p->type.buffered, p->type.persistent, p->val);
    if (eecPropertyNotCreatedDuplicateClientId == result ||
        eecPropertyNotCreatedDuplicateCloudName == result)
    {
        LOG_Warning("%s: duplicate %s", p->name, LSD_EnsoErrorCode_eToString(result));
        result = eecNoError;
    }
    if (eecNoError != result)
    {
        LOG_Error("%s: %s", p->name, LSD_EnsoErrorCode_eToString(result));
        return result;
    }
    if (p->handler)
    {
        result = LSD_SubscribeToDevicePropertyByAgentSideId(&_deviceId, p->id, p->group, p->handler, p->type.kind == PROPERTY_PRIVATE);
        if (result < 0)
        {
            LOG_Error("subscribe %s", p->name);
            return result;
        }
    }
    LOG_Info("%-11.11s subs: %d:%s %s", p->name, p->handler,
        p->handler == LED_DEVICE_HANDLER ? "Led" :
        p->handler == GW_HANDLER         ? "GW"  :
        p->handler == UPGRADE_HANDLER    ? "Upg" : "None",
        p->handler ? LSD_Group2s(p->group) : ""
    );
    return result;
}

#define Private    .type.kind       = PROPERTY_PRIVATE
#define Public     .type.kind       = PROPERTY_PUBLIC
#define Persistent .type.persistent = true
#define Buffered   .type.buffered   = true
#define Int(p)     .type.valueType  = evInt32,         .val[DESIRED_GROUP].int32Value    = p, .val[REPORTED_GROUP].int32Value    = p
#define Uint(p)    .type.valueType  = evUnsignedInt32, .val[DESIRED_GROUP].uint32Value   = p, .val[REPORTED_GROUP].uint32Value   = p
#define Float(p)   .type.valueType  = evFloat32,       .val[DESIRED_GROUP].float32Values = p, .val[REPORTED_GROUP].float32Values = p
#define Bool(p)    .type.valueType  = evBoolean,       .val[DESIRED_GROUP].booleanValue  = p, .val[REPORTED_GROUP].booleanValue  = p
#define String(p)  .type.valueType  = evString,        .val[DESIRED_GROUP].stringValue   = p, .val[REPORTED_GROUP].stringValue   = p
#define Blob(p)    .type.valueType  = evBlobHandle,    .val[DESIRED_GROUP].memoryHandle  = p, .val[REPORTED_GROUP].memoryHandle  = p
#define Time(p)    .type.valueType  = evTimestamp,     .val[DESIRED_GROUP].timestamp     = p, .val[REPORTED_GROUP].timestamp     = p
#define Desired    .group   = DESIRED_GROUP
#define Reported   .group   = REPORTED_GROUP
#define Led        .handler = LED_DEVICE_HANDLER
#define Upg        .handler = UPGRADE_HANDLER
#define GW         .handler = GW_HANDLER

const Prop_Init_t props[] =
{
    { PROP_OWNER_ID,         "$ownr",        Private, Persistent, Uint(COMMS_HANDLER)  },
    { PROP_TYPE_ID,          "type",         Public,              Uint(GW_DEVICE_TYPE) },
    { PROP_FWNAM_ID,         "fwnam",        Public,              Blob(gw_fwnam),      Desired, Upg   },
    { PROP_FWVER_ID,         "fwver",        Public,              String(GW_FWVER)     },
    { PROP_FWTIM_ID,         "fwtim",        Public,              String(GW_FWTIM)     },
    { PROP_WISAFE_ID,        "wsid",         Public,              String("")           },
    { PROP_STATE_ID,         "state",        Public,              Uint(GatewayStateRunning),Desired,  GW  },
    { PROP_DEVICE_STATUS_ID, "$devs",        Private, Persistent, Uint(THING_CREATED)  },
    { PROP_ONLINE_ID,        "onln",         Public,  Persistent, Uint(0),             Reported  },
    { PROP_ONLINE_SEQ_NO_ID, "onlns",        Public,              String(""),          Desired,  GW   },
    { PROP_UPGRD_NAME_ID,    "upgrd_name",   Public,  Persistent, Blob(NULL),          Desired,  Upg  },
    { PROP_UPGRD_URL_ID,     "upgrd_url",    Public,  Persistent, Blob(NULL),          Desired,  Upg  },
    { PROP_UPGRD_VER_ID,     "upgrd_ver",    Public,  Persistent, Blob(NULL),          Desired,  Upg  },
    { PROP_UPGRD_KEY_ID,     "upgrd_key",    Public,  Persistent, Blob(NULL),          Desired,  Upg  },
    { PROP_UPGRD_HASH_ID,    "upgrd_hash",   Public,  Persistent, Blob(NULL),          Desired,  Upg  },
    { PROP_UPGRD_TRGRD_ID,   "upgrd_trgrd",  Public,  Persistent, Uint(0),             Desired,  Upg  },
    { PROP_UPGRD_PROG_ID,    "upgrd_prog",   Public,  Persistent, Uint(0)              },
    { PROP_UPGRD_SIZE_ID,    "upgrd_size",   Public,  Persistent, Uint(0)              },
    { PROP_UPGRD_STATE_ID,   "upgrd_state",  Public,              Uint(0)              },
    { PROP_UPGRD_RETRY_ID,   "upgrd_retry",  Public,  Persistent, Uint(0)              },
    { PROP_UPGRD_ERROR_ID,   "upgrd_error",  Public,  Persistent, Uint(0)              },
    { PROP_UPGRD_STATUS_ID,  "upgrd_statu",  Public,  Persistent, Uint(0)              },
    { PROP_CONNECTION_ID,    "$conn",        Private,             Uint(0)              },
    { PROP_GW_RESET_ID,      "reset_trgrd",  Public,  Persistent, Uint(0),             Desired,  GW },
    { PROP_GW_REGISTERED_ID, "rgstd",        Public,              Bool(false),         Desired,  GW }

    ,
    // DEMO Fields
    { PROP_DEMO_LTMOUT_ID,   "learn_tmout",  Public,              Uint(0),             Desired,  GW },
    { PROP_DEMO_LTRGRD_ID,   "learn_trgrd",  Public,              Uint(0),             Desired,  GW },
    { PROP_DEMO_UNKN_ID,     "unkn",         Public,              Uint(0)              },
    { PROP_DEMO_TESTMODE_ID, "tstmd_trgrd",  Public,              Uint(0),             Desired,  GW },
    { PROP_DEMO_TESTRES_ID,  "tstmd_reslt",  Public,              Bool(false)          }
};


/**
 * Report the properties relating to firmware versions.
 *
 * @return True if the file was successfully read, otherwise false
 */
void GW_FirmwareProperties(void)
{
    // Firmware name may be passed from the build environment or read from a
    // file.
    if (strlen(gw_fwnam) == 0)
    {
        // Try to read the firmware name from a file.
        int fd = open(FW_VERSION_PATH, O_RDONLY);
        if (fd < 0)
        {
            LOG_Error("cannot open %s: %s", FW_VERSION_PATH, strerror(errno));
        }
        else
        {
            static char fwVersionJson[FW_VERSION_JSON_LEN];
            ssize_t bytes_read = read(fd, ((char *)fwVersionJson), FW_VERSION_JSON_LEN);
            close(fd);

            fwVersionJson[bytes_read] = '\0';
            if (bytes_read)
            {
                LOG_Trace("fw version file bytes read = %i\n|%s|", bytes_read, fwVersionJson);
                // Parse the file and make a name.
                int fw_di = 0;
                char fw_hwver[FW_HWVER_LEN] = {0};
                int fw_major = 0;
                int fw_minor = 0;
                char fw_name[FW_NAME_LEN] = {0};

                if (
                    ////////KVP_GetInt("di", &fwVersionJson[1], &fw_di) &&
                    KVP_GetInt("di", &fwVersionJson[1], (int32_t*)(&fw_di)) &&
                    KVP_GetString("hwver", &fwVersionJson[1], fw_hwver, FW_HWVER_LEN) &&
                    ////////KVP_GetInt("major", &fwVersionJson[1], &fw_major) &&
                    KVP_GetInt("major", &fwVersionJson[1], (int32_t*)(&fw_major)) &&
                    ////////KVP_GetInt("minor", &fwVersionJson[1], &fw_minor) &&
                    KVP_GetInt("minor", &fwVersionJson[1], (int32_t*)(&fw_minor)) &&
                    KVP_GetString("name", &fwVersionJson[1], fw_name, FW_NAME_LEN))
                {
                    int size = snprintf(gw_fwnam, MAX_FWNAME_LENGTH, "%i_%s_%i_%i_%s-%s",
                                        fw_di, fw_hwver, fw_major, fw_minor, fw_name, GW_FWVER);
                    if ( size < MAX_FWNAME_LENGTH)
                    {
                        // Success
                        LOG_Trace("Read fw name is %s", gw_fwnam);
                    }
                }
                else
                {
                    LOG_Trace("Could not read fields from the fwversion file");
                }
            }
        }
        if (strlen(gw_fwnam) == 0)
        {
            // Couldn't read the fw file so use the default.
            LOG_Trace("Setting fw name to %s", GW_FWNAMDEFAULT);
            strncpy(gw_fwnam, GW_FWNAMDEFAULT, MAX_FWNAME_LENGTH);
        }
    }
}

/******************************************************************************
 * Public Functions
 *****************************************************************************/
/**
 * \brief Initialise the gateway
 *
 * \return EnsoErrorCode_e
 */
EnsoErrorCode_e SYS_Initialise(void)
{
    GW_FirmwareProperties();
    LOG_Info("Firmware: name = %s version = %s time = %s", gw_fwnam, GW_FWVER, GW_FWTIM);
    // Create the gateway unique identifier
    _deviceId.childDeviceId = 0;
    _deviceId.technology    = ETHERNET_TECHNOLOGY;
    _deviceId.deviceAddress = HAL_GetMACAddress();
    _deviceId.isChild = false;
    /* Does the gateway already exist? */
    EnsoObject_t * owner = LSD_FindEnsoObjectByDeviceId(&_deviceId);
    if (owner == NULL)
    {
        // Create the gateway ensoObject
        owner = LSD_CreateEnsoObject(_deviceId);
        if (!owner)
        {
            LOG_Error("Failed to create gateway");
            return eecInternalError;
        }
        LOG_Info("gateway created");
    }
    _sysObject = owner;
    EnsoErrorCode_e result = eecNoError;
    for (int i = 0; i < sizeof props / sizeof props[0] && result == eecNoError; i++)
    {
        result = SYS_CreateProperty(&props[i]);
    }
    LSD_DumpObjectStore();

    return result;
}

/**
 * \brief Register the Gateway with AWS
 *
 * \return       EnsoErrorCode_e:      0 = success else error code
 */
EnsoErrorCode_e SYS_Start(void)
{
    EnsoErrorCode_e result = LSD_RegisterEnsoObject(_sysObject);
    if (result < 0)
    {
        LOG_Error("Failed to register gateway %s", LSD_EnsoErrorCode_eToString(result));
    }
    LOG_Info("Gateway registered");
    return result;
}

/**
 * \brief Returns the Gateway Device Identifier
 *
 * \param[out]  id      The Gateway identifier
 */
void SYS_GetDeviceId(EnsoDeviceId_t* id)
{
    if (id)
    {
        id->deviceAddress = _deviceId.deviceAddress;
        id->technology    = _deviceId.technology;
        id->childDeviceId = _deviceId.childDeviceId;
    }
}

/**
 * \brief Returns true if the device passed as an argument is the Gateway
 *
 * \param  id      The device identifier
 *
 * \return True if teh device is the gateway
 */
bool SYS_IsGateway(EnsoDeviceId_t* id)
{
    return (0 == LSD_DeviceIdCompare(id, &_deviceId)) ? true : false;
}
