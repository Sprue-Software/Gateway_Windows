#ifndef __ECOM_Messages_h
#define __ECOM_Messages_h

/*!****************************************************************************
 *
 * \file ECOM_Messages.h
 *
 * \author Evelyne Donnaes
 *
 * \brief Enso Communication Messages
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdint.h>
#include <stddef.h>

#include "LSD_Types.h"
#include "ECOM_Api.h"


/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

/**
 * Enso Messages
 */
typedef enum
{
    ECOM_DELTA_MSG =                0x01,
    ECOM_THING_STATUS =             0x02,
    ECOM_LOCAL_SHADOW_STATUS =      0x03,
    ECOM_PROPERTY_DELETED =         0x04,
    ECOM_GENERAL_PURPOSE1 =         0x05,
    ECOM_GENERAL_PURPOSE2 =         0x06,
    ECOM_GENERAL_PURPOSE3 =         0x07,
    ECOM_BUFFER_RX =                0x08,
    ECOM_COMMAND_RX =               0x09,
    ECOM_GATEWAY_STATUS =           0x0A,
    ECOM_POLL           =           0x0B,
    ECOM_CONNECT        =           0x0C,
    ECOM_DUMP_LOCAL_SHADOW =        0x0D,

    /* There may be debug messages or messages from other subsystems in this
     * namespace depending on the configuration and build.
     */
    ECOM_MAX_MESSAGES =             0xff
} EnsoMessage_e;

/**
 * \name ECOM_DeltaMessage_t
 *
 * \brief Delta Message sent to a subscriber.
 *
 */
typedef struct
{
    uint8_t messageId;
    HandlerId_e destinationId;        // The subscriber id
    EnsoDeviceId_t deviceId;          // The thing that changed
    PropertyGroup_e propertyGroup;    // The group to which the properties belong
    uint16_t numProperties;           // The number of properties in the delta
    EnsoPropertyDelta_t deltasBuffer[ECOM_MAX_DELTAS]; // The list of properties that have been changed
} ECOM_DeltaMessage_t;

/**
 * \name ECOM_GatewayStatus
 *
 * \brief GW Status Message sent to a subscriber.
 *
 */
typedef struct
{
    uint8_t messageId;
    EnsoGateWayStatus_e status;
} ECOM_GatewayStatus;

/**
 * \name ECOM_ThingStatusMessage_t
 *
 * \brief Thing status sent to a subscriber.
 */
typedef struct
{
    uint8_t messageId;
    EnsoDeviceId_t deviceId;
    EnsoDeviceStatus_e deviceStatus;
} ECOM_ThingStatusMessage_t;

/**
 * \name ECOM_LocalShadowStatusMessage_t
 *
 * \brief Local shadow status sent to a subscriber.
 */
typedef struct
{
    uint8_t messageId;
    LocalShadowStatus_e shadowStatus;
    const PropertyGroup_e propertyGroup;
} ECOM_LocalShadowStatusMessage_t;

/**
 * \name ECOM_PropertyDeletedMessage_t
 *
 * \brief property deleted indication sent to a subscriber.
 */
typedef struct
{
    uint8_t messageId;
    EnsoDeviceId_t deviceId;
    const EnsoAgentSidePropertyId_t agentSideId;
    char cloudSideId[LSD_PROPERTY_NAME_BUFFER_SIZE]; // Needed for AWS
} ECOM_PropertyDeletedMessage_t;

/**
 * \name ECOM_PollMessage_t
 *
 * \brief AWS poll message sent from timer to CommsHandler to initiate poll
 */
typedef struct
{
    uint8_t messageId;
} ECOM_PollMessage_t;

/**
 * \name ECOM_ConnectMessage_t
 *
 * \brief AWS Connect message sent from timer to CommsHandler to initiate poll
 */
typedef struct
{
    uint8_t messageId;
    void *  channel;
} ECOM_ConnectMessage_t;
#endif
