/*!****************************************************************************
*
* \file WiSafeTest_DAL.c
*
* \author James Green
*
* \brief Test Interface between WiSafe and the Local Shadow
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#include "LSD_Api.h"
#include "LSD_Types.h"
#include "WiSafe_DAL.h"
#include "LOG_Api.h"
#include "ECOM_Api.h"
#include "HAL.h"
#include "WiSafe_Main.h"

/* Locking is done at the API level, static functions inherit lock from parent functions */


#define DAL_PROPERTY_MAX_KEY_LENGTH (10)
#define UNUSED (0)


/* This structure is used to hold information about changes in property values. */
typedef struct _propertyDelta_t
{
    deviceId_t id;
    char key[DAL_PROPERTY_MAX_KEY_LENGTH + 1]; // The name of the key.
    EnsoPropertyValue_u value; // Note, not safe when used for string values.
} propertyDelta_t;



#define DBG(x...) LOG_Info(x)


/* This type is for a linked list of properties (key/value pairs). */
typedef struct _property_t
{
    propertyName_t key;
    EnsoValueType_e type;
    EnsoPropertyValue_u value;
    bool subscribed; // Whether or not delta should be triggered for this node.

    struct _property_t* next;
} property_t;

/* This type is for a linked list of devices. */
typedef struct _device_t
{
    deviceId_t id;
    property_t* reportedProperties;
    property_t* desiredProperties;

    struct _device_t* next;
} device_t;

static device_t* devices = NULL; // The list of all devices.
static Mutex_t shadowLock;


/* Forward declarations */
propertyName_t WiSafe_ConvertAgentToName(EnsoAgentSidePropertyId_t agentSideId);
EnsoAgentSidePropertyId_t  WiSafe_ConvertNameToAgent(propertyName_t name);



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
 * Internal helper to find the device record of the device with the
 * given ID.
 *
 * Note : Not thread safe, grab mutex in function that calls this one
 *
 * @param id The required ID.
 *
 * @return   A pointer to the device record, or NULL if not found.
 */
static device_t* FindDevice(deviceId_t id)
{
    /* Go through our list of devices to see if we can find the required one. */
    device_t* current = devices;
    while (current != NULL)
    {
        /* If it's the one, return it. */
        if (current->id == id)
        {
            return current;
        }

        /* On to the next. */
        current = current->next;
    }

    /* Not found. */
    return NULL;
}

/**
 * Internal helper which for the given device, finds the given property.
 *
 * Note : Not thread safe, grab mutex in function that calls this one
 *
 * @param propertyList The list of properties from the device_t structure.
 * @param name   The required property.
 *
 * @return       The property record, or NULL if not found/
 */
static property_t* FindProperty(property_t* propertyList, propertyName_t name)
{
    /* Go through the list of properties to see if we can find the required one. */
    property_t* current = propertyList;
    while (current != NULL)
    {
        /* If it's the one, return it. */
        if (strcmp(name, current->key) == 0)
        {
            return current;
        }

        /* On to the next. */
        current = current->next;
    }

    /* Not found. */
    return NULL;
}

/**
 * Internal helper to delete the given property, and free associated
 * memory.
 *
 * Note : Not thread safe, grab mutex in function that calls this one
 *
 * @param propertyList The list of properties from the device_t structure.
 * @param name   The required property.
 */
static void DeleteProperty(property_t** propertyList, propertyName_t name)
{
    /* Go through the list of properties to see if we can find the required one. */
    property_t** parentPointer = propertyList;
    property_t* current = *parentPointer;
    while (current != NULL)
    {
        /* If it's the one, return it. */
        if (strcmp(name, current->key) == 0)
        {
            /* We've found it, so first remove it from the list. */
            *parentPointer = current->next;

            /* Now free it. */
            free((void*)current->key);
            free(current);

            return;
        }

        /* On to the next. */
        parentPointer = &(current->next);
        current = current->next;
    }

    /* Not found. */
    return;
}

/**
 * Delete the given device and all its properties, and free all
 * associated memory.
 *
 * @param id The required device ID.
 */
void WiSafe_DALDeleteDevice(deviceId_t id)
{
    DBG(LOG_BLUE "Deleting device %06x.", id);

    OSAL_LockMutex(&shadowLock);

    /* Go through the list of devices to see if we can find the required one. */
    device_t** parentPointer = &(devices);
    device_t* current = devices;
    while (current != NULL)
    {
        /* If it's the one, delete it. */
        if (current->id == id)
        {
            /* Delete all the device reported properties. */
            do
            {
                if (current->reportedProperties != NULL)
                {
                    DeleteProperty(&(current->reportedProperties), current->reportedProperties->key);
                }
            } while (current->reportedProperties != NULL);

            /* Delete all the device desired properties. */
            do
            {
                if (current->desiredProperties != NULL)
                {
                    DeleteProperty(&(current->desiredProperties), current->desiredProperties->key);
                }
            } while (current->desiredProperties != NULL);

            /* Free and delete the device. */
            device_t* nextDevice = current->next;
            free(current);
            *parentPointer = nextDevice;
            break;
        }

        /* On to the next. */
        parentPointer = &(current->next);
        current = current->next;
    }

    OSAL_UnLockMutex(&shadowLock);

    return;
}

/**
 * Initialise this module.
 *
 */
void WiSafe_DALInit(void)
{
    static bool alreadyInitialised = false;

    if (!alreadyInitialised)
    {
        alreadyInitialised = true;

        OSAL_InitMutex(&shadowLock, NULL);

        LOG_Info("WiSafe Create Device");
        EnsoErrorCode_e errorCode = WiSafe_DALRegisterDevice(GATEWAY_DEVICE_ID, 0);

        if (errorCode != eecNoError)
        {
            LOG_Error("Failed to create gateway device - %s",
                      LSD_EnsoErrorCode_eToString(errorCode));
        }
    }
}

/**
 * Close this module.
 *
 */
void WiSafe_DALClose(void)
{
    /* Free everything. */
    while (devices != NULL)
    {
        /* Delete the device. */
        WiSafe_DALDeleteDevice(devices->id);
    }
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
    bool result;

    OSAL_LockMutex(&shadowLock);
    result = (FindDevice(id) != NULL);
    OSAL_UnLockMutex(&shadowLock);

    return result;
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
    (void)deviceType;

    OSAL_LockMutex(&shadowLock);

    /* See if it's already registered. */
    if (FindDevice(id) != NULL)
    {
        OSAL_UnLockMutex(&shadowLock);
        return eecEnsoObjectAlreadyRegistered;
    }

    /* It's not, so malloc a new struct. */
    device_t* newDevice = (device_t*)malloc(sizeof(device_t));

    /* Fill in the details. */
    newDevice->id = id;
    newDevice->reportedProperties = NULL;
    newDevice->desiredProperties = NULL;

    /* Add to the head of the list. */
    newDevice->next = devices;
    devices = newDevice;

    OSAL_UnLockMutex(&shadowLock);

    DBG("Registered device id=%06x", id);

    return eecNoError;
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
    EnsoErrorCode_e result = WiSafe_DALSetReportedProperty(id, agentSideId, name, type, newValue);

    if (result != eecNoError)
    {
        DBG(LOG_RED "Unable to set reported property %s", name);
    }
    else
    {
        result = WiSafe_DALSetDesiredProperty(id, agentSideId, name, type, newValue);

        if (result != eecNoError)
        {
            DBG(LOG_RED "Unable to set desired property %s", name);
        }
    }

    return result;
}


/**
 * Helper to allocate new delta messages.
 *
 * @return The new message, or NULL.
 */
/*
static propertyDelta_t* allocateNewDeltaMessage(void)
{
    radioCommsBuffer_t* messageBuffer = WiSafe_RadioCommsBufferGet();
    propertyDelta_t* message = (propertyDelta_t*)messageBuffer;

    assert(sizeof(propertyDelta_t) <= RADIOCOMMSBUFFER_LENGTH); // We cast comms buffers to propertyDelta. So check there is enough memory allocated in the data part of the comms buffer to do so.

    if (message == NULL)
    {
        LOG_Warning("DAL unable to allocate delta message buffer - delta will be lost.");
    }

    return message;
}
*/
/**
 * Helper to set a property value in the given list.
 *
 * Note : Not thread safe, grab mutex in function that calls this one
 *
 * @param id Device ID.
 * @param propertyList List of properties to update.
 * @param name Name of property to set.
 * @param type Type of property.
 * @param newValue Value of property to set.
 *
 * @return An error code value.
 */
static EnsoErrorCode_e WiSafe_DALSetProperty(const char* logHint, deviceId_t id,
        property_t** propertyList, propertyName_t name, EnsoValueType_e type,
        EnsoPropertyValue_u newValue)
{
    /* Check the type is one that this code can handle. */
    if ((type == evFloat32) || (type == evBlobHandle))
    {
        assert(false);
        return eecFunctionalityNotSupported;
    }

    /* See if the property already exists, and if so take note of its "subscribed" field. */
    property_t* existing = FindProperty(*propertyList, name);
    bool subscribed = false;
    if (existing != NULL)
    {
        subscribed = existing->subscribed;
    }

    /* Delete any existing property and free resources. */
    DeleteProperty(propertyList, name);

    /* Malloc space for the new property value. */
    property_t* property = (property_t*)malloc(sizeof(property_t));
    propertyName_t nameCopy = (propertyName_t)malloc(strlen(name) + 1);

    /* Fill in the details. */
    strcpy((char*)nameCopy, name);
    property->key = nameCopy;
    property->type = type;
    property->value = newValue;
    property->subscribed = subscribed;

    /* For string value need to copy the data. */
    if (type == evString)
    {
        strncpy(property->value.stringValue, newValue.stringValue, (LSD_STRING_PROPERTY_MAX_LENGTH - 1));
    }

    /* Add this property to the list. */
    property->next = *propertyList;
    *propertyList = property;

    switch (type)
    {
        case evInt32:
            DBG(LOG_BLUE "Set %s property '%s' on device id=%06x to %d", logHint, name, id, property->value.int32Value);
            break;
        case evUnsignedInt32:
            DBG(LOG_BLUE "Set %s property '%s' on device id=%06x to %u", logHint, name, id, property->value.uint32Value);
            break;
        case evBoolean:
            DBG(LOG_BLUE "Set %s property '%s' on device id=%06x to %s", logHint, name, id, property->value.booleanValue?"true":"false");
            break;
        case evString:
            DBG(LOG_BLUE "Set %s property '%s' on device id=%06x to '%s'", logHint, name, id, property->value.stringValue);
            break;
        case evTimestamp:
            DBG(LOG_BLUE "Set %s property '%s' on device id=%06x to '%u', isValid=%s", logHint, name, id, property->value.timestamp.seconds, property->value.timestamp.isValid?"true":"false");
            break;
        default:
            DBG(LOG_BLUE "Set %s property '%s' on device id=%06x", logHint, name, id);
            break;
    }

    return eecNoError;
}

/**
 * Set the 'reported' value for the given property with the given type
 * on the given device. If a desired counterpart doesn't exist as
 * well, for example the first time a property is created, then this
 * function will create one.
 *
 * @param id The required device ID.
 * @param name The name of the property to set.
 * @param type The type of the property (used when creating for the first time).
 * @param newValue The value to set.
 *
 * @return An error return code.
 */
EnsoErrorCode_e WiSafe_DALSetReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, EnsoValueType_e type, EnsoPropertyValue_u newValue)
{
    OSAL_LockMutex(&shadowLock);

    /* Find the device. */
    device_t* device = FindDevice(id);
    if (device == NULL)
    {
        /* Device not found. */
        OSAL_UnLockMutex(&shadowLock);
        return eecEnsoObjectNotFound;
    }

    /* Set the reported property. */
    EnsoErrorCode_e result = WiSafe_DALSetProperty("reported", id, &(device->reportedProperties), name, type, newValue);

    if (result == eecNoError)
    {
        /* See if there is a desired counterpart. */
        property_t* existing = FindProperty(device->desiredProperties, name);
        OSAL_UnLockMutex(&shadowLock);

        if (existing == NULL)
        {
            result = WiSafe_DALSetDesiredProperty(id, agentSideId, name, type, newValue);
        }
    }
    else
    {
        OSAL_UnLockMutex(&shadowLock);
    }

    return result;
}

/**
 * Set the 'desired' value for the given property with the given type
 * on the given device.
 *
 * @param id The required device ID.
 * @param name The name of the property to set.
 * @param type The type of the property (used when creating for the first time).
 * @param newValue The value to set.
 *
 * @return An error return code.
 */
EnsoErrorCode_e WiSafe_DALSetDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name, EnsoValueType_e type, EnsoPropertyValue_u newValue)
{
    OSAL_LockMutex(&shadowLock);

    /* Find the device. */
    device_t* device = FindDevice(id);
    if (device == NULL)
    {
        /* Device not found. */
        OSAL_UnLockMutex(&shadowLock);
        return eecEnsoObjectNotFound;
    }

    EnsoErrorCode_e errorCode = WiSafe_DALSetProperty("desired", id,
            &(device->desiredProperties), name, type, newValue);

    if (errorCode == eecNoError)
    {
        /* Are we subscribed? */
        property_t* existing = FindProperty(device->desiredProperties, name);

        OSAL_UnLockMutex(&shadowLock);

        if ((existing != NULL) && (existing->subscribed))
        {
            /* Send delta to message queue */
            EnsoPropertyDelta_t delta[ECOM_MAX_DELTAS];

            delta[0].agentSidePropertyID = agentSideId;
            delta[0].propertyValue = newValue;

            ECOM_SendUpdateToSubscriber(WISAFE_DEVICE_HANDLER, EnsoDeviceFromWiSafeID(id), DESIRED_GROUP, 1, delta);
        }
    }
    else
    {
        OSAL_UnLockMutex(&shadowLock);
    }

    return errorCode;
}



/**
 * Get the 'desired' value of the given property on the given device.
 *
 * @param id The required device ID.
 * @param name The name of the required property.
 * @param error Storage for a return error.
 *
 * @return The value read (check error to see if valid).
 */
EnsoPropertyValue_u WiSafe_DALGetDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoErrorCode_e* error)
{
    EnsoPropertyValue_u result;
    result.int32Value = 0;
    *error = eecPropertyNotFound;

    propertyName_t name = WiSafe_ConvertAgentToName(agentSideId);

    if (name == NULL)
    {
        return result;
    }

    OSAL_LockMutex(&shadowLock);

    /* First try to find the property. */
    device_t* device = FindDevice(id);
    if (device != NULL)
    {
        property_t* property = FindProperty(device->desiredProperties, name);
        if (property != NULL)
        {
            *error = eecNoError;
            result = property->value;
        }
    }

    OSAL_UnLockMutex(&shadowLock);

    return result;
}



/**
 * Get the 'reported' value of the given property on the given device.
 *
 * Note : Not thread safe, grab mutex in function that calls this one
 *
 * @param id The required device ID.
 * @param agent The agent ID.
 * @param error Storage for a return error.
 *
 * @return The value read (check error to see if valid).
 */
static EnsoPropertyValue_u WiSafe_DALGetReportedPropertyInternal(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoErrorCode_e* error)
{
    EnsoPropertyValue_u result;
    result.int32Value = 0;

    propertyName_t name = WiSafe_ConvertAgentToName(agentSideId);

    if (name == NULL)
    {
        *error = eecPropertyNotFound;
        return result;
    }

    /* First try to find the property. */
    device_t* device = FindDevice(id);
    if (device != NULL)
    {
        property_t* property = FindProperty(device->reportedProperties, name);
        if (property != NULL)
        {
            *error = eecNoError;
            return property->value;
        }
    }

    *error = eecPropertyNotFound;
    return result;
}



/**
 * Get the 'reported' value of the given property on the given device.
 *
 * @param id The required device ID.
 * @param agent The agent ID.
 * @param error Storage for a return error.
 *
 * @return The value read (check error to see if valid).
 */
EnsoPropertyValue_u WiSafe_DALGetReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoErrorCode_e* error)
{
    EnsoPropertyValue_u result;

    OSAL_LockMutex(&shadowLock);
    result = WiSafe_DALGetReportedPropertyInternal(id, agentSideId, error);
    OSAL_UnLockMutex(&shadowLock);

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
    OSAL_LockMutex(&shadowLock);

    /* Go through our list of devices to see if we can find the required one. */
    device_t* current = devices;
    while (current != NULL)
    {
        /* See if this device has the required SID. */
        EnsoErrorCode_e error;
        EnsoPropertyValue_u sid = WiSafe_DALGetReportedPropertyInternal(current->id, DAL_PROPERTY_DEVICE_SID_IDX, &error);
        if ((error == eecNoError) && (sid.uint32Value == requiredSid))
        {
            *result = current->id;
            OSAL_UnLockMutex(&shadowLock);
            return true;
        }

        /* On to the next. */
        current = current->next;
    }

    OSAL_UnLockMutex(&shadowLock);

    /* Not found. */
    return false;
}

/**
 * Iterate over devices. Returns true if a valid device is returned in
 * 'next', otherwise false.
 *
 * @param start The start device (usually the previous 'next'), or
 *              NULL to begin at the start.
 * @param next  Storage for providing the result.
 *
 * @return      True if next is valid.
 */
bool WiSafe_DALDeviceIterate(deviceId_t* start, deviceId_t* next)
{
    OSAL_LockMutex(&shadowLock);

    bool result = false;

    /* Are we starting at the very start? */
    if (start == NULL)
    {
        if (devices != NULL)
        {
            *next = devices->id;
            result = true;
        }
        else
        {
            result = false;
        }
    }
    else
    {
        /* Try to find the start device. */
        device_t* startDevice = FindDevice(*start);
        if (startDevice == NULL)
        {
            /* Not found. */
            result = false;
        }
        else
        {
            /* Return next. */
            if (startDevice->next != NULL)
            {
                *next = startDevice->next->id;
                result = true;
            }
            else
            {
                result = false;
            }
        }
    }

    OSAL_UnLockMutex(&shadowLock);

    return result;
}

/**
 * Subscribe to the given DESIRED property and start receiving delta
 * messages for any changes.
 *
 * @param id The device ID.
 * @param name The name of the desired property.
 *
 * @return Standard error code.
 */
EnsoErrorCode_e WiSafe_DALSubscribeToDesiredProperty(deviceId_t id, EnsoAgentSidePropertyId_t pid)
{
    propertyName_t name = WiSafe_ConvertAgentToName(pid);
    assert(name != NULL);

    if (name == NULL)
    {
        /* Device not found. */
        return eecEnsoObjectNotFound;
    }

    OSAL_LockMutex(&shadowLock);

    /* Find the device. */
    device_t* device = FindDevice(id);
    if (device == NULL)
    {
        OSAL_UnLockMutex(&shadowLock);
        /* Device not found. */
        return eecEnsoObjectNotFound;
    }

    /* Find the property. */
    property_t* existing = FindProperty(device->desiredProperties, name);
    if (existing == NULL)
    {
        OSAL_UnLockMutex(&shadowLock);
        /* Property not found. */
        return eecEnsoObjectNotFound;
    }

    OSAL_UnLockMutex(&shadowLock);

    existing->subscribed = true;
    return eecNoError;
}

/**
 * Helper to increment the value of a property. Assumes the type of
 * the property is a U32.
 *
 * @param id The device ID.
 * @param name The property key.
 *
 * @return Standard error code.
 */
EnsoErrorCode_e WiSafe_DALIncrementReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, propertyName_t name)
{
    EnsoErrorCode_e result;
    EnsoPropertyValue_u count = WiSafe_DALGetReportedProperty(id, agentSideId, &result);

    EnsoPropertyValue_u value;

    if (result == eecNoError)
    {
        /* Increment value. */
        value.uint32Value = count.uint32Value + 1;
        result = WiSafe_DALSetReportedProperty(id, agentSideId, name, evUnsignedInt32, value);
    }
    else
    {
        /* Not found, so this is the first time, so create the property. */
        value.uint32Value = 1;
        result = WiSafe_DALSetReportedProperty(id, agentSideId, name, evUnsignedInt32, value);
    }

    return result;
}


/* Added these directly from WiSafe_DAL.c */

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
    EnsoErrorCode_e error = eecNoError;
    device_t* current = devices;
    *numDevices = 0;

    while ((current != NULL) && (*numDevices < size))
    {
        resultBuffer[*numDevices] = current->id;
        *numDevices += 1;
        current = current->next;
    }

    return error;
}

EnsoErrorCode_e WiSafe_DALSetReportedProperties(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId[],
        propertyName_t name[], EnsoValueType_e type[], EnsoPropertyValue_u newValue[], int numProperties, PropertyKind_e kind,bool buffered, bool persistent)
{
    EnsoErrorCode_e errorCode = eecNoError;

    for (int i=0; i<numProperties; i++)
    {
        errorCode = WiSafe_DALSetReportedProperty(id, agentSideId[i], name[i], type[i], newValue[i]);

        if (errorCode != eecNoError)
        {
            LOG_Error("Failed to set reported for group of properties? %s",
                    LSD_EnsoErrorCode_eToString(errorCode));
            break;
        }
    }

    return errorCode;
}

EnsoErrorCode_e WiSafe_DALSetDesiredProperties(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId[],
        propertyName_t name[], EnsoValueType_e type[], EnsoPropertyValue_u newValue[], int numProperties, PropertyKind_e kind, bool buffered, bool persistent)
{
    EnsoErrorCode_e errorCode = eecNoError;

    OSAL_LockMutex(&shadowLock);

    /* Find the device. */
    device_t* device = FindDevice(id);
    if (device == NULL)
    {
        /* Device not found. */
        OSAL_UnLockMutex(&shadowLock);
        return eecEnsoObjectNotFound;
    }

    /* First update the shadow (atomically) with all the changes. */
    for (int i=0; i<numProperties; i++)
    {
        errorCode = WiSafe_DALSetProperty("desired", id, &(device->desiredProperties), name[i], type[i], newValue[i]);

        if (errorCode != eecNoError)
        {
            LOG_Error("Failed to set reported for group of properties? %s", LSD_EnsoErrorCode_eToString(errorCode));
        }
    }

    /* Now send a single delta message with all the changes. */
    EnsoPropertyDelta_t delta[ECOM_MAX_DELTAS];

    for (int i=0; i<numProperties; i++)
    {
        delta[i].agentSidePropertyID = agentSideId[i];
        delta[i].propertyValue = newValue[i];
    }

    ECOM_SendUpdateToSubscriber(WISAFE_DEVICE_HANDLER, EnsoDeviceFromWiSafeID(id), DESIRED_GROUP, numProperties, delta);
    OSAL_UnLockMutex(&shadowLock);

    return errorCode;
}

propertyName_t WiSafe_ConvertAgentToName(EnsoAgentSidePropertyId_t agentSideId)
{
    switch (agentSideId)
    {
        case DAL_PROPERTY_IN_NETWORK_IDX: return DAL_PROPERTY_IN_NETWORK;
        case DAL_PROPERTY_UNKNOWN_SIDS_IDX: return DAL_PROPERTY_UNKNOWN_SIDS;
        case DAL_PROPERTY_DEVICE_TEST_TIMESTAMP_IDX: return DAL_PROPERTY_DEVICE_TEST_TIMESTAMP;
        case DAL_PROPERTY_DEVICE_SID_IDX: return DAL_PROPERTY_DEVICE_SID;
        case DAL_PROPERTY_DEVICE_MODEL_IDX: return DAL_PROPERTY_DEVICE_MODEL;
        case DAL_PROPERTY_DEVICE_ALARM_TEMPV_IDX: return DAL_PROPERTY_DEVICE_ALARM_TEMPV;
        case DAL_PROPERTY_DEVICE_ALARM_STATE_IDX: return DAL_PROPERTY_DEVICE_ALARM_STATE;
        case DAL_PROPERTY_DEVICE_BATTERY_VOLTAGE_IDX: return DAL_PROPERTY_DEVICE_BATTERY_VOLTAGE;
        case DAL_PROPERTY_DEVICE_ALARM_SEQ_IDX: return DAL_PROPERTY_DEVICE_ALARM_SEQ;
        case DAL_PROPERTY_DEVICE_TEMPERATURE_IDX: return DAL_PROPERTY_DEVICE_TEMPERATURE;
        case DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT_IDX: return DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT;
        case DAL_PROPERTY_DEVICE_RADIO_RSSI_IDX: return DAL_PROPERTY_DEVICE_RADIO_RSSI;
        case DAL_PROPERTY_DEVICE_RM_SD_FAULT_IDX: return DAL_PROPERTY_DEVICE_RM_SD_FAULT;
        case DAL_PROPERTY_DEVICE_LAST_SEQUENCE_IDX: return DAL_PROPERTY_DEVICE_LAST_SEQUENCE;
        case DAL_COMMAND_DELETE_ALL_IDX: return DAL_COMMAND_DELETE_ALL;
        case DAL_COMMAND_TEST_MODE_IDX: return DAL_COMMAND_TEST_MODE;
        case DAL_COMMAND_TEST_MODE_FLUSH_IDX: return DAL_COMMAND_TEST_MODE_FLUSH;
        case DAL_COMMAND_TEST_MODE_RESULT_IDX: return DAL_COMMAND_TEST_MODE_RESULT;
        case DAL_COMMAND_LEARN_IDX: return DAL_COMMAND_LEARN;
        case DAL_COMMAND_LEARN_TIMEOUT_IDX: return DAL_COMMAND_LEARN_TIMEOUT;
        case DAL_COMMAND_FLUSH_IDX: return DAL_COMMAND_FLUSH;
        case DAL_COMMAND_DEVICE_FLUSH_IDX: return DAL_COMMAND_DEVICE_FLUSH;
        case DAL_COMMAND_SOUNDER_TEST_IDX: return DAL_COMMAND_SOUNDER_TEST;
        case DAL_PROPERTY_DEVICE_MUTE_IDX: return DAL_PROPERTY_DEVICE_MUTE;
        case PROP_STATE_ID: return PROP_STATE_CLOUD_NAME;
        case PROP_MANUFACTURER_ID: return PROP_MANUFACTURER_CLOUD_NAME;
        case PROP_ONLINE_ID: return PROP_ONLINE_CLOUD_NAME;
        case PROP_MODEL_ID: return PROP_MODEL_CLOUD_NAME;
        case PROP_TYPE_ID: return PROP_TYPE_CLOUD_NAME;

        default:
            if ((agentSideId >= DAL_PROPERTY_DEVICE_FAULT_STATE_IDX) && (agentSideId < DAL_PROPERTY_DEVICE_FAULT_SEQ_IDX))
            {
                static char result[15]; // "flt<SIA code>_state";
                ////////sprintf(result, "flt%d_state", (agentSideId - DAL_PROPERTY_DEVICE_FAULT_STATE_IDX));	//////// [RE:cast]
                sprintf(result, "flt%d_state", (int)(agentSideId - DAL_PROPERTY_DEVICE_FAULT_STATE_IDX));
                return result;
            }
            else if ((agentSideId >= DAL_PROPERTY_DEVICE_FAULT_SEQ_IDX) && (agentSideId < DAL_PROPERTY_DEVICE_FAULT_TIME_IDX))
            {
                static char result[15]; // "flt<SIA code>_seq";
                ////////sprintf(result, "flt%d_seq", (agentSideId - DAL_PROPERTY_DEVICE_FAULT_SEQ_IDX));	//////// [RE:cast]
                sprintf(result, "flt%d_seq", (int)(agentSideId - DAL_PROPERTY_DEVICE_FAULT_SEQ_IDX));
                return result;
            }
            else
            {
                LOG_Error("Unable to find name for agent ID 0x%08x", agentSideId);
                return NULL;
            }
    }
}


EnsoAgentSidePropertyId_t  WiSafe_ConvertNameToAgent(propertyName_t name)
{
    if (name == NULL)
    {
        LOG_Error("Passed in NULL");
        return 0;
    }

    if (strcmp(name, DAL_PROPERTY_IN_NETWORK) == 0)
    {
        return DAL_PROPERTY_IN_NETWORK_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_UNKNOWN_SIDS) == 0)
    {
        return DAL_PROPERTY_UNKNOWN_SIDS_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_TEST_TIMESTAMP) == 0)
    {
        return DAL_PROPERTY_DEVICE_TEST_TIMESTAMP_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_SID) == 0)
    {
        return DAL_PROPERTY_DEVICE_SID_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_MODEL) == 0)
    {
        return DAL_PROPERTY_DEVICE_MODEL_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_ALARM_TEMPV) == 0)
    {
        return DAL_PROPERTY_DEVICE_ALARM_TEMPV_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_ALARM_TEMPV) == 0)
    {
        return DAL_PROPERTY_DEVICE_ALARM_TEMPV_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_ALARM_STATE) == 0)
    {
        return DAL_PROPERTY_DEVICE_ALARM_STATE_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_BATTERY_VOLTAGE) == 0)
    {
        return DAL_PROPERTY_DEVICE_BATTERY_VOLTAGE_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_ALARM_SEQ) == 0)
    {
        return DAL_PROPERTY_DEVICE_ALARM_SEQ_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_TEMPERATURE) == 0)
    {
        return DAL_PROPERTY_DEVICE_TEMPERATURE_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT) == 0)
    {
        return DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_RADIO_RSSI) == 0)
    {
        return DAL_PROPERTY_DEVICE_RADIO_RSSI_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_RM_SD_FAULT) == 0)
    {
        return DAL_PROPERTY_DEVICE_RM_SD_FAULT_IDX;
    }
    else if (strcmp(name, DAL_PROPERTY_DEVICE_LAST_SEQUENCE) == 0)
    {
        return DAL_PROPERTY_DEVICE_LAST_SEQUENCE_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_DELETE_ALL) == 0)
    {
        return DAL_COMMAND_DELETE_ALL_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_TEST_MODE) == 0)
    {
        return DAL_COMMAND_TEST_MODE_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_TEST_MODE_FLUSH) == 0)
    {
        return DAL_COMMAND_TEST_MODE_FLUSH_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_TEST_MODE_RESULT) == 0)
    {
        return DAL_COMMAND_TEST_MODE_RESULT_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_LEARN) == 0)
    {
        return DAL_COMMAND_LEARN_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_LEARN_TIMEOUT) == 0)
    {
        return DAL_COMMAND_LEARN_TIMEOUT_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_FLUSH) == 0)
    {
        return DAL_COMMAND_FLUSH_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_DEVICE_FLUSH) == 0)
    {
        return DAL_COMMAND_DEVICE_FLUSH_IDX;
    }
    else if (strcmp(name, DAL_COMMAND_SOUNDER_TEST) == 0)
    {
        return DAL_COMMAND_SOUNDER_TEST_IDX;
    }

    LOG_Error("Unable to find agent ID for %s", name);
    return 0;
}

/**
 * Check that the given property equals the given value.
 */
bool CheckReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoValueType_e type, EnsoPropertyValue_u requiredValue)
{
    bool result = false;
    OSAL_LockMutex(&shadowLock);

    EnsoErrorCode_e error;
    EnsoPropertyValue_u actualValue = WiSafe_DALGetReportedPropertyInternal(id, agentSideId, &error);

    if (error != eecNoError)
    {
        LOG_Error("WiSafe_DALGetReportedProperty error (%s), agentSideId=%08x, id=%06x.", LSD_EnsoErrorCode_eToString(error), agentSideId, id);
    }

    switch (type)
    {
        case evInt32:
            result = (requiredValue.int32Value == actualValue.int32Value);
            break;
        case evUnsignedInt32:
            result = (requiredValue.uint32Value == actualValue.uint32Value);
            break;
        case evBoolean:
            result = (requiredValue.booleanValue == actualValue.booleanValue);
            break;
        case evString:
            result = (strcmp(requiredValue.stringValue, actualValue.stringValue) == 0);
            break;
        case evTimestamp:
            result = (requiredValue.timestamp.seconds == actualValue.timestamp.seconds);
            break;
        default:
            result = false;
            break;
    }

    OSAL_UnLockMutex(&shadowLock);

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
    DBG(LOG_BLUE "Deleting property '%s' on device id=%016llx", name, id);

    OSAL_LockMutex(&shadowLock);

    /* Find the device. */
    device_t* device = FindDevice(id);
    if (device == NULL)
    {
        /* Device not found. */
        OSAL_UnLockMutex(&shadowLock);
        return eecEnsoObjectNotFound;
    }

    /* See if the property already existed. */
    EnsoErrorCode_e result = eecNoError;
    property_t* existing = FindProperty(device->reportedProperties, name);
    if (existing == NULL)
    {
        result = eecPropertyNotFound;
    }

    /* Reported property. */
    DeleteProperty(&(device->reportedProperties), name);

    /* Desired property. */
    DeleteProperty(&(device->desiredProperties), name);

    OSAL_UnLockMutex(&shadowLock);

    if ((result != eecNoError) && (result != eecPropertyNotFound))
    {
        LOG_Error("Failed to delete property '%s'.", name);
    }
    return result;
}
