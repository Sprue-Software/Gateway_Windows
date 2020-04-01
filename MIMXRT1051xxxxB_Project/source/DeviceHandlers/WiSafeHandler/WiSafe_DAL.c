/*!****************************************************************************
*
* \file WiSafe_DAL.c
*
* \author James Green
*
* \brief Interface between WiSafe and the Local Shadow
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
//#include <inttypes.h>

#include "LSD_Types.h"
#include "LOG_Api.h"
#include "LSD_Api.h"
#include "APP_Types.h"
#include "HAL.h"

#include "WiSafe_DAL.h"
#include "WiSafe_Main.h"

#define DAL_DBG(x...) LOG_InfoC(x)


/**
 * Helper to decide whether this property is persistent or not
 *
 * @param agentSideId Agent side ID.
 *
 * @return true if it is persistent or false otherwise
 */
static bool WiSafe_DALIsPropertyPersistent(EnsoAgentSidePropertyId_t agentSideId)
{
    return DAL_PROPERTY_DEVICE_SID_IDX     == agentSideId ||
           DAL_COMMAND_DELETE_ALL_IDX      == agentSideId ||
           DAL_COMMAND_FLUSH_IDX           == agentSideId ||
           DAL_COMMAND_LEARN_IDX           == agentSideId ||
           DAL_COMMAND_LEARN_TIMEOUT_IDX   == agentSideId ||
           DAL_COMMAND_SOUNDER_TEST_IDX    == agentSideId ||
           DAL_COMMAND_TEST_MODE_IDX       == agentSideId ||
           DAL_COMMAND_TEST_MODE_FLUSH_IDX == agentSideId ||
           DAL_PROPERTY_UNKNOWN_SIDS_IDX   == agentSideId ||
           PROP_TYPE_ID                    == agentSideId ||
           PROP_MANUFACTURER_ID            == agentSideId ||
           PROP_ONLINE_ID                  == agentSideId ||
           PROP_MODEL_ID                   == agentSideId ||
           DAL_PROPERTY_DEVICE_MODEL_IDX   == agentSideId ||
           DAL_PROPERTY_DEVICE_MUTE_IDX    == agentSideId;
}

/**
 * Helper to decide the kind of a property
 *
 * @param agentSideId Agent side ID.
 *
 * @return PROPERTY_PUBLIC or PROPERTY_PRIVATE;
 */
static PropertyKind_e WiSafe_DALPropertyKind(EnsoAgentSidePropertyId_t agentSideId)
{
    return (DAL_PROPERTY_DEVICE_SID_IDX           != agentSideId &&
            DAL_PROPERTY_DEVICE_LAST_SEQUENCE_IDX != agentSideId &&
            DAL_PROPERTY_DEVICE_MODEL_IDX         != agentSideId &&
            DAL_PROPERTY_DEVICE_MISSING_IDX       != agentSideId &&
            DAL_PROPERTY_INTERROGATING_IDX        != agentSideId )
            ? PROPERTY_PUBLIC : PROPERTY_PRIVATE;
}

/**
 * Helper to convert from a WiSafe device ID to an AWS Thing ID.
 *
 * @param id WiSafe device ID.
 *
 * @return AWS Thing ID.
 */
EnsoDeviceId_t EnsoDeviceFromWiSafeID(deviceId_t id)
{
    EnsoDeviceId_t result;

    if (id == GATEWAY_DEVICE_ID)
    {
        result.childDeviceId = 0;
        result.technology = ETHERNET_TECHNOLOGY;
        result.deviceAddress = HAL_GetMACAddress();
        result.isChild = false;
    }
    else
    {
        result.deviceAddress = id;
        result.technology = WISAFE_TECHNOLOGY;
        result.childDeviceId = 0;
        result.isChild = true;
    }

    return result;
}

/**
 * Delete the given device and all its properties, and free all
 * associated memory.
 *
 * @param id The required device ID.
 */
void WiSafe_DALDeleteDevice(deviceId_t id)
{
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);
    DAL_DBG(LOG_BLUE "Deleting device id=%016llx", ensoId.deviceAddress);
    LSD_DestroyEnsoDevice(ensoId);
    return;
}

/**
 * Initialise this module.
 *
 */
void WiSafe_DALInit(void)
{
}

/**
 * Close this module.
 *
 */
void WiSafe_DALClose(void)
{
}

/**
 * Determine if the device with the given ID is already in the shadow.
 *
 * @param id The required device ID.
 *
 * @return True if already registerd, false if not.
 */
bool WiSafe_DALIsDeviceRegistered(deviceId_t id)
{
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);
    EnsoObject_t* existing = LSD_FindEnsoObjectByDeviceId(&ensoId);
    return (existing != NULL);
}

/**
 * Register the device.
 *
 * @param id The ID of the device to register.
 *
 * @return An error return code.
 */
EnsoErrorCode_e WiSafe_DALRegisterDevice(deviceId_t id, uint32_t deviceType)
{
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);

    EnsoObject_t* newObject = LSD_CreateEnsoObject(ensoId);
    if (newObject != NULL)
    {
        // Create the owner (persistent private) property.
        EnsoPropertyValue_u handlerValue[PROPERTY_GROUP_MAX];
        handlerValue[DESIRED_GROUP].uint32Value = WISAFE_DEVICE_HANDLER;
        handlerValue[REPORTED_GROUP].uint32Value = WISAFE_DEVICE_HANDLER;

        EnsoErrorCode_e result = LSD_CreateProperty(
                newObject,
                PROP_OWNER_ID,
                PROP_OWNER_INTERNAL_NAME,
                evUnsignedInt32,
                PROPERTY_PRIVATE,
                false,
                true,
                handlerValue);
        if (result < 0)
        {
            LOG_Error("Failed to create owner property %s", LSD_EnsoErrorCode_eToString(result));
        }

        // Create the device status private property.
        EnsoPropertyValue_u deviceStatusValue[PROPERTY_GROUP_MAX];
        deviceStatusValue[DESIRED_GROUP].uint32Value = THING_CREATED;
        deviceStatusValue[REPORTED_GROUP].uint32Value = THING_CREATED;

        result = LSD_CreateProperty(
                newObject,
                PROP_DEVICE_STATUS_ID,
                PROP_DEVICE_STATUS_CLOUD_NAME,
                evUnsignedInt32,
                PROPERTY_PRIVATE,
                false,
                false,
                deviceStatusValue);
        if (result < 0)
        {
            LOG_Error("Failed to create device status property %s", LSD_EnsoErrorCode_eToString(result));
        }

        // Create the connection (private) property.
        EnsoPropertyValue_u connValue[PROPERTY_GROUP_MAX];
        connValue[DESIRED_GROUP].int32Value = -1;
        connValue[REPORTED_GROUP].int32Value = -1;

        result = LSD_CreateProperty(
                newObject,
                PROP_CONNECTION_ID,
                PROP_CONNECTION_ID_CLOUD_NAME,
                evInt32,
                PROPERTY_PRIVATE,
                false,
                false,
                connValue);
        if (result < 0)
        {
            LOG_Error("Failed to create connection property %s", LSD_EnsoErrorCode_eToString(result));
        }

        // Create the type (public) property.
        EnsoPropertyValue_u typeValue[PROPERTY_GROUP_MAX];
        typeValue[DESIRED_GROUP].uint32Value = deviceType;
        typeValue[REPORTED_GROUP].uint32Value = deviceType;

        bool bTypePersistent = WiSafe_DALIsPropertyPersistent(PROP_TYPE_ID);
        result = LSD_CreateProperty(
                newObject,
                PROP_TYPE_ID,
                PROP_TYPE_CLOUD_NAME,
                evUnsignedInt32,
                PROPERTY_PUBLIC,
                false,
                bTypePersistent,
                typeValue);
        if (result < 0)
        {
            LOG_Error("Failed to create type property %s", LSD_EnsoErrorCode_eToString(result));
        }

        result = LSD_RegisterEnsoObject(newObject);
        return result;
    }
    else
    {
        return eecEnsoObjectAlreadyRegistered;
    }
}

/**
 * Helper to log a set operation to the console.
 * @param logHint Additional log output.
 * @param id The required device ID.
 * @param name The name of the property to set.
 * @param agentSideId Agent side ID.
 * @param type The type of the property (used when creating for the first time).
 * @param newValue The value to set.
 */
static void LogSetOperation(const char* logHint, deviceId_t id, propertyName_t name, EnsoAgentSidePropertyId_t agentSideId, EnsoValueType_e type, EnsoPropertyValue_u newValue)
{
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);

    switch (type)
    {
        case evInt32:
            DAL_DBG(LOG_BLUE "Setting %s property '%s' (%x) on device id=%016llx to %d", logHint, name, agentSideId, ensoId.deviceAddress, newValue.int32Value);
            break;
        case evUnsignedInt32:
            DAL_DBG(LOG_BLUE "Setting %s property '%s' (%x) on device id=%016llx to %u", logHint, name, agentSideId, ensoId.deviceAddress, newValue.uint32Value);
            break;
        case evBoolean:
            DAL_DBG(LOG_BLUE "Setting %s property '%s' (%x) on device id=%016llx to %s", logHint, name, agentSideId, ensoId.deviceAddress, newValue.booleanValue?"true":"false");
            break;
        case evString:
            DAL_DBG(LOG_BLUE "Setting %s property '%s' (%x) on device id=%016llx to '%s'", logHint, name, agentSideId, ensoId.deviceAddress, newValue.stringValue);
            break;
        case evTimestamp:
            DAL_DBG(LOG_BLUE "Setting %s property '%s' (%x) on device id=%016llx to %d", logHint, name, agentSideId, ensoId.deviceAddress, newValue.timestamp.seconds);
            break;
        default:
            DAL_DBG(LOG_BLUE "Setting %s property '%s' (%x) on device id=%016llx", logHint, name, agentSideId, ensoId.deviceAddress);
            break;
    }
}

/**
 * Create a property if it does not already exist and set a value for the
 * given property with the given type on the given device.
 *
 * @param logHint Additional log output.
 * @param id The required device ID.
 * @param agentSideId Agent side ID.
 * @param name The name of the property to set.
 * @param group The group to set.
 * @param type The type of the property (used when creating for the first time).
 * @param newValue The value to set.
 *
 * @return An error return code.
 */
static EnsoErrorCode_e SetPropertyHelper(const char* logHint, deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, PropertyGroup_e group, EnsoValueType_e type, EnsoPropertyValue_u newValue)
{
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);

    /* Tracing. */
    LogSetOperation(logHint, id, name, agentSideId, type, newValue);

    /* Create a record for the new property. */
    EnsoPropertyValue_u value[PROPERTY_GROUP_MAX];
    value[DESIRED_GROUP] = newValue;
    value[REPORTED_GROUP] = newValue;

    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;

    /* Find the owner. */
    EnsoObject_t* owner = LSD_FindEnsoObjectByDeviceId(&ensoId);

    if (owner != NULL)
    {
        bool persistent = WiSafe_DALIsPropertyPersistent(agentSideId);

        PropertyKind_e kind = WiSafe_DALPropertyKind(agentSideId);

        /* Try to create the property; we will know if it already exists because we'll get an error. */
        retVal = LSD_CreateProperty(owner, agentSideId, name, type, kind,
                false, persistent, value);

        /* Did the property already exist? If so, just set it. */
        if ((retVal == eecPropertyNotCreatedDuplicateClientId) || (retVal == eecPropertyNotCreatedDuplicateCloudName))
        {
            retVal = LSD_SetPropertyValueByAgentSideId(
                    WISAFE_DEVICE_HANDLER,
                    &ensoId,
                    group,
                    agentSideId,
                    newValue);
        }

        if (retVal != eecNoError)
        {
            DAL_DBG(LOG_RED "Unable to create/set group of properties - %s",
                    LSD_EnsoErrorCode_eToString(retVal));
        }
    }

    return retVal;
}

/**
 * Create or set a group of properties. The set must be of same group, type, buffered and persistence.
 *
 * @param logHint Additional log output.
 * @param id The required device ID.
 * @param agentSideId Array of agent side IDs.
 * @param name Array of cloud names for the property.
 * @param group The group to set.
 * @param type Array of types for the property (used when creating for the first time).
 * @param newValue Array of values to set.
 * @param newProperties Number of properties in the array.
 * @param kind Public or private.
 * @param buffered Are the properties buffered.
 * @param persistent Are the properties persistent.
 *
 * @return An error return code.
 */
static EnsoErrorCode_e SetPropertiesHelper(const char* logHint, deviceId_t id,
        EnsoAgentSidePropertyId_t agentSideId[], propertyName_t name[], PropertyGroup_e group,
        EnsoValueType_e type[], EnsoPropertyValue_u newValue[], int numProperties, PropertyKind_e kind,
        bool buffered, bool persistent)
{
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);

    /* Tracing */
    DAL_DBG(LOG_BLUE "Setting set of %i %s properties as a group", numProperties, logHint);

    /* Create a record for the new properties and a delta, we'll need one or the other. */
    EnsoPropertyValue_u values[numProperties][PROPERTY_GROUP_MAX];
    EnsoPropertyDelta_t delta[numProperties];
    for (int i=0; i<numProperties; i++)
    {
        values[i][DESIRED_GROUP] = newValue[i];
        values[i][REPORTED_GROUP] = newValue[i];

        delta[i].agentSidePropertyID = agentSideId[i];
        delta[i].propertyValue = newValue[i];

        LogSetOperation(logHint, id, name[i], agentSideId[i], type[i], newValue[i]);
    }

    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;

    /* Find the owner. */
    EnsoObject_t* owner = LSD_FindEnsoObjectByDeviceId(&ensoId);

    if (owner != NULL)
    {
        retVal = LSD_CreateProperties(owner, agentSideId, name, type, kind, buffered, persistent, values, numProperties);

        /* Did the property already exist? If so, just set it. */
        if ((retVal == eecPropertyNotCreatedDuplicateClientId) ||
            (retVal == eecPropertyNotCreatedDuplicateCloudName))
        {
            retVal = LSD_SetPropertiesOfDevice(WISAFE_DEVICE_HANDLER, &ensoId, group, delta, numProperties);
        }

        if (retVal != eecNoError)
        {
            DAL_DBG(LOG_RED "Unable to create/set group of properties - %s",
                    LSD_EnsoErrorCode_eToString(retVal));
        }
    }

    return retVal;
}

/**
 * Try and create a property, if it already exists then no problem as it has been
 * restored from a previous run and we do not want to overwrite the value
 *
 * @param id The required device ID.
 * @param agentSideId Agent side ID.
 * @param name The name of the property to set.
 * @param type The type of the property (used when creating for the first time).
 * @param newValue The value to set.
 *
 * @return An error return code.
 */
EnsoErrorCode_e WiSafe_DALCreateProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, EnsoValueType_e type, EnsoPropertyValue_u newValue)
{
    EnsoErrorCode_e result = eecEnsoObjectNotFound;

    /* Find the owner. */
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);
    EnsoObject_t* owner = LSD_FindEnsoObjectByDeviceId(&ensoId);

    LogSetOperation("create", id, name, agentSideId, type, newValue);

    if (owner != NULL)
    {
        bool persistent = WiSafe_DALIsPropertyPersistent(agentSideId);

        PropertyKind_e kind = WiSafe_DALPropertyKind(agentSideId);

        EnsoPropertyValue_u values[PROPERTY_GROUP_MAX] = {newValue, newValue};

        result = LSD_CreateProperty(owner, agentSideId, name, type, kind, false, persistent, values);

        if ((result == eecPropertyNotCreatedDuplicateClientId) || (result == eecPropertyNotCreatedDuplicateCloudName))
        {
            result = eecNoError;
        }
        else if (result != eecNoError)
        {
            LOG_Error("Failed to create property %s (%x)", name, agentSideId);
        }
    }
    else
    {
        LOG_Error("Failed to find EnsoObject?");
    }

    return result;
}

/**
 * Try to delete a property.
 *
 * @param id The required device ID.
 * @param agentSideId Agent side ID.
 * @param name The name of the property to set.
 *
 * @return An error return code.
 */
extern EnsoErrorCode_e WiSafe_DALDeleteProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name)
{
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);

    DAL_DBG(LOG_BLUE "Deleting property '%s' on device id=%016llx", name, ensoId.deviceAddress);

    EnsoErrorCode_e result = LSD_RemovePropertyByAgentSideId(&ensoId, agentSideId);

    if ((result != eecNoError) && (result != eecPropertyNotFound))
    {
        LOG_Error("Failed to delete property '%s'.", name);
    }

    return result;
}

/**
 * Set the reported value for the given property with the given type
 * on the given device. If a desired counterpart doesn't exist as
 * well, for example the first time a property is created, then this
 * function will create one.
 *
 * @param id The required device ID.
 * @param agentSideId Agent side ID.
 * @param name The name of the property to set.
 * @param type The type of the property (used when creating for the first time).
 * @param newValue The value to set.
 *
 * @return An error return code.
 */
EnsoErrorCode_e WiSafe_DALSetReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, EnsoValueType_e type, EnsoPropertyValue_u newValue)
{
    return SetPropertyHelper("reported", id, agentSideId, name, REPORTED_GROUP, type, newValue);
}

/**
 * Set the reported value for the given property with the given type
 * on the given device. If a desired counterpart doesn't exist as
 * well, for example the first time a property is created, then this
 * function will create one.
 *
 * @param id The required device ID.
 * @param agentSideId Agent side ID.
 * @param name The name of the property to set.
 * @param type The type of the property (used when creating for the first time).
 * @param newValue The value to set.
 * @param numProperties The number of properties in the group.
 * @param kind public or private
 * @param buffered Buffered or not
 * @param persistent Persistent or not
 * @return An error return code.
 */
EnsoErrorCode_e WiSafe_DALSetReportedProperties(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId[], propertyName_t name[], EnsoValueType_e type[], EnsoPropertyValue_u newValue[], int numProperties, PropertyKind_e kind, bool buffered, bool persistent)
{
    return SetPropertiesHelper("reported", id, agentSideId, name, REPORTED_GROUP, type, newValue, numProperties, kind, buffered, persistent);
}

/**
 * Set the desired value for the given property with the given type
 * on the given device.
 *
 * @param id The required device ID.
 * @param agentSideId Agent side ID.
 * @param name The name of the property to set.
 * @param type The type of the property (used when creating for the first time).
 * @param newValue The value to set.
 *
 * @return An error return code.
 */
EnsoErrorCode_e WiSafe_DALSetDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, EnsoValueType_e type, EnsoPropertyValue_u newValue)
{
    return SetPropertyHelper("desired", id, agentSideId, name, DESIRED_GROUP, type, newValue);
}

/**
 * Get the reported value of the given property on the given device.
 *
 * @param id The required device ID.
 * @param agentSideId The property agent side ID.
 * @param error Storage for a return error.
 *
 * @return The value read (check error to see if valid).
 */
EnsoPropertyValue_u WiSafe_DALGetReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoErrorCode_e* error)
{
    EnsoPropertyValue_u result;
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);
    *error = LSD_GetPropertyValueByAgentSideId(&ensoId, REPORTED_GROUP, agentSideId, &result);
    return result;
}

/**
 * Get the desired value of the given property on the given device.
 *
 * @param id The required device ID.
 * @param agentSideId The property agent side ID.
 * @param error Storage for a return error.
 *
 * @return The value read (check error to see if valid).
 */
EnsoPropertyValue_u WiSafe_DALGetDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoErrorCode_e* error)
{
    EnsoPropertyValue_u result;
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);
    *error = LSD_GetPropertyValueByAgentSideId(&ensoId, DESIRED_GROUP, agentSideId, &result);
    return result;
}

/**
 * Get the device ID (if known) for the device with the given SID.
 *
 * @param requiredSid The required SID.
 * @param result Storage for the result.
 *
 * @return True if result written, else false.
 */
bool WiSafe_DALGetDeviceIdForSID(uint8_t requiredSid, deviceId_t* result)
{
    EnsoDeviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
    HandlerId_e handlerId = WISAFE_DEVICE_HANDLER;
    uint16_t numDevices = 0;
    EnsoErrorCode_e error = LSD_GetDevicesId(handlerId, buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);

    if (error == eecNoError)
    {
        for (int loop = 0; loop < numDevices; loop += 1)
        {
            EnsoPropertyValue_u sid;
            EnsoErrorCode_e error = LSD_GetPropertyValueByAgentSideId(&(buffer[loop]), REPORTED_GROUP, DAL_PROPERTY_DEVICE_SID_IDX, &sid);
            if ((error == eecNoError) && (sid.uint32Value == requiredSid))
            {
                /* We've found it. So get the device ID. */
                *result = buffer[loop].deviceAddress;
                return true;
            }
        }
    }

    /* Not found. */
    return false;
}

/**
 * Request a list of all WiSafe devices.
 *
 * @param resultBuffer The buffer to store the list.
 * @param size The size of the buffer.
 * @param numDevices Return value for number of entries actually written to buffer.
 *
 * @return A standard error code.
 */
EnsoErrorCode_e WiSafe_DALDevicesEnumerate(deviceId_t* resultBuffer, uint16_t size, uint16_t* numDevices)
{
    EnsoDeviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
    HandlerId_e handlerId = WISAFE_DEVICE_HANDLER;
    EnsoErrorCode_e error = LSD_GetDevicesId(handlerId, buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, numDevices);

    if (error == eecNoError)
    {
        if (*numDevices > size)
        {
            *numDevices = size;
        }

        for (uint16_t loop = 0; loop < *numDevices; loop += 1)
        {
            resultBuffer[loop] = (deviceId_t)(buffer[loop].deviceAddress);

            LOG_Info("WiSafe_DALDevicesEnumerate %016"PRIx64"_%04x_%02x",
                    buffer[loop].deviceAddress,
                    buffer[loop].technology,
                    buffer[loop].childDeviceId);

        }
    }

    return error;
}

/**
 * Subscribe to the given DESIRED property and start receiving delta
 * messages for any changes.
 *
 * @param id The device ID.
 * @param pid The property ID.
 *
 * @return Standard error code.
 */
EnsoErrorCode_e WiSafe_DALSubscribeToDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t pid)
{
    EnsoDeviceId_t ensoId = EnsoDeviceFromWiSafeID(id);

    EnsoErrorCode_e error = LSD_SubscribeToDevicePropertyByAgentSideId(
            &ensoId,
            pid,
            DESIRED_GROUP,
            WISAFE_DEVICE_HANDLER,
            true);

    return error;
}

/**
 * Helper to increment the value of a property. Assumes the type of
 * the property is a U32.
 *
 * @param id The device ID.
 * @param agentSideId Agent side ID.
 * @param name The property key.
 *
 * @return Standard error code.
 */
EnsoErrorCode_e WiSafe_DALIncrementReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name)
{
    EnsoErrorCode_e result;
    EnsoPropertyValue_u count = WiSafe_DALGetReportedProperty(id, agentSideId, &result);
    if (result == eecNoError)
    {
        /* Increment value. */
        EnsoPropertyValue_u value = { .uint32Value = (count.uint32Value + 1) };
        result = WiSafe_DALSetReportedProperty(id, agentSideId, name, evUnsignedInt32, value);
    }
    else
    {
        /* Not found, so this is the first time, so create the property. */
        EnsoPropertyValue_u value = { .uint32Value = 1 };
        result = WiSafe_DALSetReportedProperty(id, agentSideId, name, evUnsignedInt32, value);
    }

    return result;
}
