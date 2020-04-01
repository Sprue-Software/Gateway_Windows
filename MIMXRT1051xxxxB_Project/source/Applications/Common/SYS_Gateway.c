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

/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

static EnsoDeviceId_t _deviceId;
static EnsoObject_t* _sysObject;

#ifndef GW_FWNAM
#define GW_FWNAM "" // Read the firmware name from the operating system later
#endif

char gw_fwnam[MAX_FWNAME_LENGTH + 1] = GW_FWNAM;

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
    { PROP_STATE_ID,         "state",        Public,              Uint(GatewayStateRunning) },
    { PROP_DEVICE_STATUS_ID, "$devs",        Private, Persistent, Uint(THING_CREATED)  },
    { PROP_ONLINE_ID,        "onln",         Public,  Persistent, Uint(0),             Reported  },
    { PROP_ONLINE_SEQ_NO_ID, "onlns",        Public,  Persistent, String(""),          Desired,  GW   },
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
    { PROP_GW_REGISTERED_ID, "rgstd",        Public,              Bool(false),         Desired,  GW },
    { PROP_CERT_MANAGER_URL_ID, "cmurl",     Public,              Blob(NULL),          Desired,  GW }
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
        int fd = open(FW_VERSION_PATH, 1);//nishi
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

                ////////if (KVP_GetInt("di", &fwVersionJson[1], &fw_di) &&	//////// [RE:cast]
                if (KVP_GetInt("di", &fwVersionJson[1], (int32_t*)(&fw_di)) &&
                    KVP_GetString("hwver", &fwVersionJson[1], fw_hwver, FW_HWVER_LEN) &&
                    ////////KVP_GetInt("major", &fwVersionJson[1], &fw_major) &&	//////// [RE:cast]
                    KVP_GetInt("major", &fwVersionJson[1], (int32_t*)(&fw_major)) &&
                    ////////KVP_GetInt("minor", &fwVersionJson[1], &fw_minor) &&	//////// [RE:cast]
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

    /*
     * SYS_Start() registers device and this triggers a subscription.
     * Checking the UnAnnounced objects also finds the GW as announced device
     * at startup so it also tries to subscribe GW again.
     * So, duplicated subscriptions for GW which cause to get notifications from
     * Cloud two times.
     * To avoid this issue, lets set announceInProgress flag to skip
     * "Unnannounced Objects" step for GW while SYS_Start() already triggers to
     * subscribe GW.
     */
    owner->announceInProgress = true;

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
