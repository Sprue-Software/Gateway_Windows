#ifndef __LSD_TYPES
#define __LSD_TYPES

/*!****************************************************************************
 *
 * \file LSD_Types.h
 *
 * \author Carl Pickering
 *
 * \brief Type definitions for ensoAgent3
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "C:/Users/ndiwathe/Documents/MCUXpressoIDE_11.1.1_3241/workspace/EnsoAgent/source/OSAL/OSAL_Types.h"

/*!****************************************************************************
 * Constants
 *****************************************************************************/

// Enso Object name is of the form <mac>_<technology>_00
//                              or <mac>_<technology>_00_<mac>_<technology>_00
#define ENSO_OBJECT_NAME_MAX_LENGTH 49
#define ENSO_OBJECT_NAME_BUFFER_SIZE (ENSO_OBJECT_NAME_MAX_LENGTH + 1)

#define ENSO_OBJECT_STRING_MAX_LENGTH 16
#define ENSO_OBJECT_STRING_BUFFER_SIZE (ENSO_OBJECT_STRING_MAX_LENGTH + 1)

// A simple property name has a maximum of 5 chars
#define LSD_SIMPLE_PROPERTY_NAME_MAX_LENGTH 5
// A nested property name is of the form: "prop1_propA"
#define LSD_PROPERTY_MAX_NESTING_LEVEL 1
#define LSD_PROPERTY_NAME_MAX_LENGTH (LSD_SIMPLE_PROPERTY_NAME_MAX_LENGTH * (LSD_PROPERTY_MAX_NESTING_LEVEL + 1) + LSD_PROPERTY_MAX_NESTING_LEVEL)
#define LSD_PROPERTY_NAME_BUFFER_SIZE (LSD_PROPERTY_NAME_MAX_LENGTH + 1)

// Define maximum string property size, string will include null which
// means there will be length-1 useful characters
#define LSD_STRING_PROPERTY_MAX_LENGTH 12


/**
 * \name LSD_STATIC
 *
 * \brief Used for unit testing where we want to expose certain static
 * variables or functions to the unit test but keep them private at run-time.
 * This requires LSD_UNIT_TESTS to be defined in the makefile to drop out
 * static so the default for production is to leave them private.
 *
 */
#ifndef LSD_STATIC
    #define LSD_STATIC static
#endif

/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

typedef float float32_t;

/**
 * \name EnsoType_t
 *
 * \brief The enso object type.
 */
typedef uint16_t EnsoType_t;

/**
 * \name EnsoIndex_t
 *
 * \brief Used for indices within the ensoAgent.
 */
typedef int16_t EnsoIndex_t;

/**
 * \name EnsoHandle_t
 *
 * \brief Base enso handle
 */
//typedef EnsoIndex_t EnsoHandle_t;

/**
 * \name EnsoMemoryHandle_t
 *
 * \brief Handle for strings and other lumps of memory stored by the ensoClient
 */

typedef MemoryHandle_t EnsoMemoryHandle_t;

/**
 * \name EnsoErrorCode_t
 *
 * \brief An enumeration used to pass back error codes.
 */
typedef enum EnsoErrorCode_tag
{
    eecPropertyNotFound = -32768,
    eecFunctionalityNotSupported,
    eecNullPointerSupplied,
    eecParameterOutOfRange,
    eecPropertyValueNotObjectHandle,
    eecPropertyObjectHandleNotValue,
    eecPropertyNotCreatedDuplicateClientId,
    eecPropertyNotCreatedDuplicateCloudName,
    eecPropertyWrongType,
    eecPropertyGroupNotSupported,
    eecEnsoObjectNotFound,
    eecEnsoDeviceNotFound,
    eecSubscriberNotFound,
    eecEnsoObjectAlreadyRegistered,
    eecBufferTooSmall,
    eecBufferTooBig,
    eecConversionFailed,
    eecPoolFull,
    eecTimeOut,
    eecEnsoObjectNotCreated,
    eecLocalShadowConnectionFailedToInitialise,
    eecLocalShadowConnectionFailedToConnect,
    eecLocalShadowConnectionFailedToSetAutoConnect,
    eecLocalShadowConnectionFailedToClose,
    eecLocalShadowConnectionPollingFailed,
    eecLocalShadowFailedSubscription,
    eecLocalShadowDocumentFailedToInitialise,
    eecLocalShadowDocumentFailedFinalise,
    eecLocalShadowDocumentFailedToSendDelta,
    eecTestFailed,
    eecInternalError,
    eecWouldBlock,
    eecOpenFailed,
    eecCloseFailed,
    eecReadFailed,
    eecWriteFailed,
    eecRenameFailed,
    eecRemoveFailed,
    eecDeviceNotSupported,
    eecNoChange,
    eecMQTTClientBusy,
    eecNoError = 0
} EnsoErrorCode_e;


typedef struct {
    uint32_t seconds;
    bool isValid;
} EnsoTimestamp;

typedef union EnsoPropertyValue_tag
{
    int32_t int32Value;
    uint32_t uint32Value;
    float32_t float32Value;
    bool booleanValue;
    EnsoMemoryHandle_t memoryHandle;
    char stringValue[LSD_STRING_PROPERTY_MAX_LENGTH];
    EnsoTimestamp timestamp;
} EnsoPropertyValue_u;

/**
 * \name EnsoAgentSidePropertyId_t
 *
 * \brief The property ID as supplied by the Device Abstraction Layer
 *
 *
 */
typedef uint32_t EnsoAgentSidePropertyId_t;

/**
 *\name EnsoPropertyDelta_t
 *
 *\brief Details of change of single property
 *
 * This is passed in the notification to identify the property and what its
 * new value is.
 *
 */

typedef struct EnsoPropertyDelta_tag
{
    EnsoPropertyValue_u propertyValue;
    EnsoAgentSidePropertyId_t agentSidePropertyID;
} EnsoPropertyDelta_t;

typedef enum PropertyGroup_enum
{
    DESIRED_GROUP = 0,
    REPORTED_GROUP,
    PROPERTY_GROUP_MAX
} PropertyGroup_e;

typedef enum EnsoValueType_etag
{
    evInt32 = 0,
    evUnsignedInt32 = 1,
    evFloat32 = 2,
    evBoolean = 3,
    evString = 4,
    evBlobHandle = 5,
    evTimestamp = 6,
    evNumObjectTypes = 7
} EnsoValueType_e;

/**
 * Property access mode bitmap - ReadOnly, WriteOnly, ReadWrite
 */
typedef enum PropertyAccessMode_etag
{
    ACCESS_MODE_INVALID = 0,
    ACCESS_MODE_RO = 1,
    ACCESS_MODE_WO,
    ACCESS_MODE_RW
} PropertyAccessMode_e;

/**
 * Property type
 * Note: this is a 2-bit field in EnsoProperty_t
 */
typedef enum
{
    PROPERTY_PRIVATE = 0,   /* Default. Local to enso agent i.e. not published to the cloud */
    PROPERTY_PUBLIC = 1     /* Published to the cloud */
} PropertyKind_e;

typedef uint32_t SubscriptionBitmap_t;

typedef struct
{
    bool reportedOutOfSync    : 1;   // Reported value is not synced with cloud shadow
    bool desiredOutOfSync     : 1;   // Desired  value is not synced with cloud Shadow
    bool buffered             : 1;   // Buffering is enabled for this property (default: non-buffered)
    bool persistent           : 1;   // Persistent data                        (default: non-persistent)
    PropertyKind_e  kind      : 2;   // Public, private                        (default: private)
    ////////EnsoValueType_e valueType : 26;  // To align on a word boundary	//////// [RE:type]
    EnsoValueType_e valueType : 8;  // To align on a word boundary
} EnsoPropertyType_t;

typedef struct EnsoProperty_tag
{
    SubscriptionBitmap_t      subscriptionBitmap[PROPERTY_GROUP_MAX];
    EnsoPropertyType_t        type;
    EnsoAgentSidePropertyId_t agentSidePropertyID;
    EnsoPropertyValue_u       desiredValue;
    EnsoPropertyValue_u       reportedValue;
    char                      cloudName[LSD_PROPERTY_NAME_BUFFER_SIZE];
} EnsoProperty_t;


/**
 * Handler identifier
 */
typedef enum
{
    INVALID_HANDLER = 0,

    /* Logical entities */
    COMMS_HANDLER =  1,
    UPGRADE_HANDLER,
    STORAGE_HANDLER,
    AUTOMATION_ENGINE_HANDLER,
    LOG_HANDLER,
    TIMESTAMP_HANDLER,
    LSD_HANDLER,
    GW_HANDLER,

    /* Device handler entities */
    LED_DEVICE_HANDLER,
    WISAFE_DEVICE_HANDLER,
    TEST_DEVICE_HANDLER,
    TELEGESIS_DEVICE_HANDLER,

    ENSO_HANDLER_MAX = 32
} HandlerId_e;


/**
 * Technology identifier
 */
typedef enum
{
    WISAFE_TECHNOLOGY = 0,
    ZIGBEE_TECHNOLOGY,
    ETHERNET_TECHNOLOGY,
    TECHNOLOGY_MAX
} Technology_e;


/**
 * Device identifier
 */
typedef struct EnsoDeviceId_tag
{
    uint64_t          deviceAddress;             /* Unique device address, e.g. ZigBee IEEE address */
    uint16_t          technology;
    uint8_t           childDeviceId;             /* The child device identifier */
    bool              isChild;
} EnsoDeviceId_t;


/**
 * Device power identification
 */
typedef enum
{
    DEVICE_POWER_MAINS = 0,
    DEVICE_POWER_BATTERY,
    DEVICE_POWER_EMERGENCY,
    DEVICE_POWER_NUM_TYPES
} DevicePowerSource_e;


typedef char EnsoDeviceName_t[ENSO_OBJECT_NAME_BUFFER_SIZE];

/*
 * GW Status
 */
typedef enum
{
    GATEWAY_NOT_REGISTERED,
    GATEWAY_REGISTERED
} EnsoGateWayStatus_e;

/**
 * Thing status
 */
typedef enum
{
    THING_DELETED,
    THING_CREATED,
    THING_DISCOVERED,
    THING_ACCEPTED,
    THING_REJECTED
} EnsoDeviceStatus_e;


typedef char EnsoString_t[ENSO_OBJECT_STRING_BUFFER_SIZE];


/**
 * \brief Enso Object is the representation of a device in the local shadow.
 *
 */
typedef struct EnsoObject_tag
{
    EnsoDeviceId_t            deviceId;
    bool                      announceAccepted;
    bool                      announceInProgress;
    // Flags to indicate whether object has out-of-sync properties with AWS Shadow
    bool                      reportedOutOfSync;
    bool                      desiredOutOfSync;
    EnsoIndex_t               propertyListStart;
    int32_t                   unsubscriptionRetryCount;
    SubscriptionBitmap_t      subscriptionBitmap[PROPERTY_GROUP_MAX];
} EnsoObject_t;

/**
 * Local Shadow status
 */
typedef enum
{
    LSD_SYNC_COMPLETED,
    LSD_DUMP_COMPLETED
} LocalShadowStatus_e;

/**
 * Property filter for sending property state t a handler
 */
typedef enum
{
    PROPERTY_FILTER_OUT_OF_SYNC,
    PROPERTY_FILTER_PERSISTENT,
    PROPERTY_FILTER_TIMESTAMPS,
    PROPERTY_FILTER_ALL
} PropertyFilter_e;

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e LSD_ConvertTypedDataToJsonValue(
        char* destBuffer,
        int destBufferSize,
        int* bufferUsed,
        const EnsoPropertyValue_u propVal,
        const EnsoValueType_e type );

EnsoErrorCode_e LSD_GetThingName(
        char* destBuffer,
        int destBufferSize,
        int* bufferUsed,
        const EnsoDeviceId_t deviceId,
        const EnsoDeviceId_t parentId);

EnsoErrorCode_e LSD_GetThingFromNameString(
        const char* sourceBuffer,
        EnsoDeviceId_t *rxThing);

char * LSD_EnsoErrorCode_eToString(EnsoErrorCode_e error);
char * LSD_ValueType2s(EnsoValueType_e type);
char * LSD_Value2s(char * buf, int size, EnsoValueType_e type, EnsoPropertyValue_u * pval);
char * LSD_Group2s(PropertyGroup_e group);
#endif
