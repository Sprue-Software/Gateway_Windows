/*!****************************************************************************
 *
 * \file     LSD_PropertyStore.c
 *
 * \author   Carl Pickering
 *
 * \brief    The property store for the ensoAgent
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "LSD_EnsoObjectStore.h"
#include "LSD_PropertyStore.h"
#include "LSD_Subscribe.h"
#include "LOG_Api.h"
#include "OSAL_Api.h"
#include "ECOM_Api.h"


/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

/**
 * \name prv_PropertyStore
 *
 * \brief This is the property pool
 */
LSD_STATIC EnsoPropertyStore_t prv_PropertyStore;


/**
 *
 * \brief Pointer to the unique local shadow mutex
 */
static Mutex_t * pLSD_Mutex = NULL;


/*!****************************************************************************
 * Functions
 *****************************************************************************/

/**
 * \name prv_GetPropertyIndexByClientSideId
 *
 * NOT THREAD SAFE
 *
 * \brief This function searches the property list for the ensoObject to find
 * the property that matches the supplied client side ID.
 *
 * \param   propertyStore       Pointer to the property store containing the
 *                              property lists.
 *
 * \param   ownerObject         The ensoObject to which the property belongs
 *
 * \param   clientSideId        The numerical ID of the property to look for.
 *
 * \param[out] propertyIndex    The index of the property
 *
 * \return                      EnsoErrorCode
 *
 */
LSD_STATIC EnsoErrorCode_e prv_GetPropertyIndexByClientSideId(
        EnsoPropertyStore_t* propertyStore,
        const EnsoObject_t* ownerObject,
        const EnsoAgentSidePropertyId_t clientSideId,
        int*  propertyIndex)
{

    /* A bit of sanity checking ... */
    if ( ! ( propertyStore  &&  ownerObject &&  propertyIndex) )
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    int i = ownerObject->propertyListStart;
    *propertyIndex = -1;
    EnsoErrorCode_e retVal = eecPropertyNotFound;

    while ((i >= 0) && (retVal < 0))
    {
        if (clientSideId == propertyStore->propertyPool[i].property.agentSidePropertyID)
        {
            /* Found it */
            *propertyIndex = i;
            retVal = eecNoError;
        }
        i = propertyStore->propertyPool[i].nextContainerIndex;
    }

    return retVal;
}

/**
 * \name prv_GetPropertyIndexByCloudName
 *
 * NOT THREAD SAFE
 *
 * \brief This function searches the property list for the ensoObject to find
 * the property that matches the supplied cloud side ID
 *
 * \param   propertyStore       Pointer to the property store containing the
 *                              property lists.
 *
 * \param   ownerObject         The ensoObject to which the property belongs
 *
 * \param   cloudName           The string identifier of the property to look
 *                              for.
 *
 * \param[out] propertyIndex    The index of the property
 *
 * \return                      EnsoErrorCode
 *
 */
LSD_STATIC EnsoErrorCode_e prv_GetPropertyIndexByCloudName(
        EnsoPropertyStore_t* propertyStore,
        const EnsoObject_t* ownerObject,
        const char* cloudName,
        int* propertyIndex)
{
    EnsoErrorCode_e retVal = eecPropertyNotFound;
    int i;

    /* A bit of sanity checking ... */
    if (NULL == propertyStore)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }
    if (NULL == ownerObject)
    {
        LOG_Error("null pointer input param");
        return eecEnsoObjectNotFound;
    }
    if (NULL == cloudName)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }
    if (NULL == propertyIndex)
    {
        LOG_Error("null pointer input param");
    }

    i = ownerObject->propertyListStart;
    *propertyIndex = -1;

    while ((i >= 0) && (retVal < 0))
    {
        if (0 == strcmp(propertyStore->propertyPool[i].property.cloudName, cloudName))
        {
            /* Found it */
            *propertyIndex = i;
            retVal =eecNoError;
        }
        i = propertyStore->propertyPool[i].nextContainerIndex;
    }

    return retVal;
}

/**
 * \name    prv_InitialisePropertyStore
 *
 * \brief   This sets up an empty property store ready to populate.
 *
 * NOT THREAD SAFE
 *
 * \param   propertyStore       Pointer to the property store structure to
 *                              initialise.
 *
 * \return  ErrorCode
 *  */
LSD_STATIC EnsoErrorCode_e prv_InitialisePropertyStore(EnsoPropertyStore_t* propertyStore)
{
    int i, propertyGroup;

    if (NULL == propertyStore)
    {
        LOG_Error("null pointer input param");
    }

    memset(propertyStore, 0, sizeof(EnsoPropertyStore_t));

    propertyStore->firstFreeProperty = 0;

    for (i = 0; i < LSD_PROPERTY_POOL_SIZE; i++)
    {
        propertyStore->propertyPool[i].property.agentSidePropertyID = 0;
        propertyStore->propertyPool[i].property.cloudName[0] = 0;
        propertyStore->propertyPool[i].property.desiredValue.int32Value = 0;
        propertyStore->propertyPool[i].property.reportedValue.int32Value = 0;
        for (propertyGroup = 0; propertyGroup < PROPERTY_GROUP_MAX; propertyGroup++)
        {
            propertyStore->propertyPool[i].property.subscriptionBitmap[propertyGroup] = 0;
        }
        /* Now undefine freed element and put back into the free list */
        switch (propertyStore->propertyPool[i].property.type.valueType)
        {
        case evBlobHandle:
            propertyStore->propertyPool[i].property.desiredValue.memoryHandle = NULL;
            propertyStore->propertyPool[i].property.reportedValue.memoryHandle = NULL;
            break;

        default:
            break;
        }
        propertyStore->propertyPool[i].nextContainerIndex = i + 1;
    }

    propertyStore->propertyPool[LSD_PROPERTY_POOL_SIZE - 1].nextContainerIndex = -1;

    return eecNoError;
}

/**
 * \name        prv_CreateBlob
 *
 * \brief       Internal function to create a Blob
 *
 * \param       newValue NULL, or a null terminated string.
 *
 * \return      value -  NULL Blob, or Blob containing the string.
 */
EnsoPropertyValue_u prv_CreateBlob(EnsoPropertyValue_u newValue)
{
    EnsoPropertyValue_u value = { .memoryHandle = NULL };
    if (newValue.memoryHandle != NULL)
    {
        int size = strlen(newValue.memoryHandle) + 1;
        value.memoryHandle = OSAL_MemoryRequest(NULL, size);
        if (value.memoryHandle != NULL)
        {
            strcpy(value.memoryHandle, newValue.memoryHandle);
        }
    }
    return value;
}

/**
 * \name        prv_CreateProperty
 *
 * \brief       Internal function to create the property
 *
 *              NOT THREAD SAFE
 *
 * \param       propertyStore   The property store to add the property to
 *
 * \param       owner           The ensoObject to which the new property
 *                              belongs
 *
 * \param       agentSideId     The numeric ID used by the driver to find
 *                              this property - only has to be unique for
 *                              this owner. There is no constraint for
 *                              consistency on this ID though it's normally
 *                              generated by the driver/client based on
 *                              hardware details.
 *
 * \param       cloudSideId     The string identifier used by the cloud in
 *                              to identify this property - again it only
 *                              needs to be unique for this object.
 *
 * \param       type            The property type
 *
 * \param       kind            The property kind
 *
 * \param       buffered        Buffered or not
 *
 * \param       persistent      Persistent or not
 *
 * \param       groupValues     An array of initial values for the property -
 *                              there must be one for each property group.
 *
 * \return      Error Code
 */
LSD_STATIC EnsoErrorCode_e prv_CreateProperty(
        EnsoPropertyStore_t* propertyStore,
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId,
        const EnsoValueType_e type,
        const PropertyKind_e kind,
        const bool buffered,
        const bool persistent,
        const EnsoPropertyValue_u* groupValues)
{
    /* A bit of sanity checking ... */
    if (!owner || !cloudSideId || !groupValues)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    if (0 == agentSideId)
    {
        return eecParameterOutOfRange;
    }

    size_t cloudSideIdLength = strlen( cloudSideId );
    if (cloudSideIdLength > LSD_PROPERTY_NAME_MAX_LENGTH)
    {
        return eecParameterOutOfRange;
    }

    if (type < 0 || type >= evNumObjectTypes)
    {
        return eecParameterOutOfRange;
    }

    /* Make sure a property with either the specified cloudSideId or agentSideId
     * is not already in the store. */

    int index;
    if (prv_GetPropertyIndexByClientSideId(propertyStore, owner, agentSideId, &index) >= 0)
    {
        // There is a property with this agentSideId.
        // Is a cloud name provided ?
        if (strlen(cloudSideId))
        {
            EnsoProperty_t * theProperty = &(propertyStore->propertyPool[index].property);
            if (strlen(theProperty->cloudName) == 0)
            {
                LOG_Info(LOG_GREEN "Setting cloudName %s for prop id %x", cloudSideId, agentSideId);
                // The cloud name is empty so this property must have been populated from persistent
                // storage at startup. Set the cloud name.
                strncpy(theProperty->cloudName, cloudSideId, LSD_PROPERTY_NAME_BUFFER_SIZE);
            }
        }
        return eecPropertyNotCreatedDuplicateClientId;
    }
    if (strlen(cloudSideId) && prv_GetPropertyIndexByCloudName(propertyStore, owner, cloudSideId, &index) >= 0)
    {
        return eecPropertyNotCreatedDuplicateCloudName;
    }

    EnsoErrorCode_e retVal = eecNoError;
    int new_prop_index = propertyStore->firstFreeProperty;
    int old_first_index = owner->propertyListStart;

    if (0 <= new_prop_index)
    {
        /* Move firstFreeProperty onto the next item in the property list
         * it doesn't matter if it's an end of list as that will still
         * count as a valid value. */
        propertyStore->firstFreeProperty = propertyStore->propertyPool[new_prop_index].nextContainerIndex;

        EnsoProperty_t * theProperty = &(propertyStore->propertyPool[new_prop_index].property);
        memset(theProperty, 0, sizeof(EnsoProperty_t));

        theProperty->type.kind = kind;
        theProperty->type.buffered = buffered;
        theProperty->type.persistent = persistent;
        theProperty->type.valueType = type;
        theProperty->agentSidePropertyID = agentSideId;
        if (type == evBlobHandle)
        {
            theProperty->desiredValue  = prv_CreateBlob(groupValues[DESIRED_GROUP]);
            theProperty->reportedValue = prv_CreateBlob(groupValues[REPORTED_GROUP]);
        }
        else
        {
            theProperty->desiredValue  = groupValues[DESIRED_GROUP];
            theProperty->reportedValue = groupValues[REPORTED_GROUP];
        }
        strncpy(theProperty->cloudName, cloudSideId, LSD_PROPERTY_NAME_BUFFER_SIZE);
        propertyStore->propertyPool[new_prop_index].nextContainerIndex = old_first_index;
        owner->propertyListStart = new_prop_index;
        if (kind == PROPERTY_PUBLIC)
        {
            // By definition they will be out of sync when created and public
            theProperty->type.desiredOutOfSync = false;
            theProperty->type.reportedOutOfSync = true;
            owner->desiredOutOfSync = false;
            owner->reportedOutOfSync = true;
        }
        else
        {
            theProperty->type.desiredOutOfSync = false;
            theProperty->type.reportedOutOfSync = false;
        }

        retVal = eecNoError;
    }
    else
    {
        /* For some reason we couldn't create a new property - probably
         * out of space
         */
        retVal = eecPoolFull;
    }

    return retVal;
}

/**
 * \name        LSD_RemoveProperty_Safe
 *
 * \brief       Internal function used to remove a property from the pool
 *              either by agent side it or by cloud name
 *
 * \param       deviceId        The device that owns the property to be removed
 *
 * \param       removeFirstProperty  If True then the device first property is removed
 *                                   If False then the property identified by agentSideId or cloudName is removed
 *
 * \param       agentSideId     The internal ID of the property to be removed
 *
 * \param       cloudName       The name of the property to be removed
 *
 * \param       notify          Whether to notify handlers
 *
 * \return      EnsoErrorCode_e
 */

EnsoErrorCode_e LSD_RemoveProperty_Safe(
        const EnsoDeviceId_t* deviceId,
        const bool removeFirstProperty,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudName,
        const bool notify)
{
    char cloudNameBuffer[LSD_PROPERTY_NAME_BUFFER_SIZE];
    bool isPublic = false;

    // Lock Local Shadow
    OSAL_LockMutex(pLSD_Mutex);

    EnsoErrorCode_e retVal = eecPropertyNotFound;
    EnsoObject_t* owner = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if (owner)
    {
        int i = owner->propertyListStart;
        int prevIndex = -1 ;
        while ( (i >= 0) &&  (eecPropertyNotFound == retVal) )
        {
            EnsoProperty_t * property = &prv_PropertyStore.propertyPool[i].property;
            if ( removeFirstProperty ||
                 ((cloudName   && (0 == strcmp(property->cloudName, cloudName))) ||
                  (agentSideId && (agentSideId == property->agentSidePropertyID))) )
            {
                /* Found it */

                // We need cloud name for notification below, so store it if we don't have it
                if (!cloudName)
                {
                    strncpy(cloudNameBuffer, property->cloudName, LSD_PROPERTY_NAME_BUFFER_SIZE);
                    cloudName = cloudNameBuffer;
                }

                isPublic = (property->type.kind == PROPERTY_PUBLIC);

                if (prevIndex > 0)
                {
                    // Unlink element
                    prv_PropertyStore.propertyPool[prevIndex].nextContainerIndex = prv_PropertyStore.propertyPool[i].nextContainerIndex;
                }
                else
                {
                    // It was the first item in the list
                    owner->propertyListStart = prv_PropertyStore.propertyPool[i].nextContainerIndex;
                }
                /* Now free element and put back into the free list */
                switch (property->type.valueType)
                {
                    case evBlobHandle:
                        /* Free memory associated with this value */
                        if (property->desiredValue.memoryHandle)
                        {
                            OSAL_Free(property->desiredValue.memoryHandle);
                            property->desiredValue.memoryHandle = NULL;
                        }
                        if (property->reportedValue.memoryHandle)
                        {
                            OSAL_Free(property->reportedValue.memoryHandle);
                            property->reportedValue.memoryHandle = NULL;
                        }
                        break;

                    default:
                        break;
                }
                memset(property, 0, sizeof(EnsoProperty_t));
                prv_PropertyStore.propertyPool[i].nextContainerIndex = prv_PropertyStore.firstFreeProperty;
                prv_PropertyStore.firstFreeProperty = i;
                retVal = eecNoError;
                break; // Exit while loop
            }

            prevIndex = i;
            i = prv_PropertyStore.propertyPool[i].nextContainerIndex;
        }
    }

    // Unlock local shadow
    OSAL_UnLockMutex(pLSD_Mutex);

    if (eecNoError == retVal && notify)
    {
        // Notify storage handler
        retVal = ECOM_SendPropertyDeletedToSubscriber(
                STORAGE_HANDLER,
                owner->deviceId,
                agentSideId,
                cloudName);
        if (eecNoError != retVal)
        {
            LOG_Error("Error sending ECOM_PROPERTY_DELETED to %d", STORAGE_HANDLER);
        }

        // Notify comms handler if it is a public property
        if (isPublic)
        {
            retVal = ECOM_SendPropertyDeletedToSubscriber(
                    COMMS_HANDLER,
                    owner->deviceId,
                    agentSideId,
                    cloudName);
            if (eecNoError != retVal)
            {
                LOG_Error("Error sending ECOM_PROPERTY_DELETED to %d", COMMS_HANDLER);
            }
        }
    }

    return retVal;
}


/**
 * \brief       Internal function to remove all properties from an object and
 *              return their bodies to the pool.
 *
 * \param       deviceId        The device that owns the property to be removed
 *
 * \return      EnsoErrorCode_e
 */
EnsoErrorCode_e  LSD_RemoveAllPropertiesOfObject_Safe(
        const EnsoDeviceId_t* deviceId)
{
    if (!deviceId)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    EnsoErrorCode_e retVal = eecNoError;
    while (eecNoError == retVal)
    {
        // Remove the first property and don't notify the handlers on each property deletion;
        // Instead they will be notified that the device has been deleted
        retVal = LSD_RemoveProperty_Safe(deviceId, true, 0, 0, false);
    }

    if (eecPropertyNotFound == retVal)
    {
        // All done
        retVal = eecNoError;
    }

    return retVal;
}

/**
 * \name        prv_SetProperty
 *
 * \brief       Internal function for setting a property value used internally
 *              by the get and set functions for values and buffers. It's just
 *              here to avoid code duplication.
 *
 *              NOT THREAD SAFE
 *
 * \param       property        The property to update
 *
 * \param       group           The group of the property to set
 *
 * \param       value           The value to set the property to
 *
 * \return      Error Code
 *
 */
LSD_STATIC EnsoErrorCode_e prv_SetProperty(
        const EnsoProperty_t *property,
        PropertyGroup_e group,
        EnsoPropertyValue_u value)
{
    if (!property )
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }
    if (group < 0 || group >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }
    const char * name = property->cloudName;
    EnsoPropertyValue_u* ptrValue = LSD_GetPropertyValuePtrFromGroupDirectly(property, group);
    if (!ptrValue)
    {
        return eecInternalError;
    }
    bool changed = true;
    char * gstr = LSD_Group2s(group);
    switch (property->type.valueType)
    {
    case evInt32:
        changed = value.int32Value != ptrValue->int32Value;
#if VERBOSE_PROPERTY_STORE_DEBUG
        LOG_Trace("%s %s old: %d, new: %d %schanged", name, gstr, ptrValue->int32Value, value.int32Value, changed ? "" : "un");
#endif
        break;
    case evUnsignedInt32:
        changed = value.uint32Value != ptrValue->uint32Value;
#if VERBOSE_PROPERTY_STORE_DEBUG
        LOG_Trace("%s %s old: %u, new: %u %schanged", name, gstr, ptrValue->uint32Value, value.uint32Value, changed ? "" : "un");
#endif
        break;
    case evFloat32:
        changed = value.float32Value != ptrValue->float32Value;
#if VERBOSE_PROPERTY_STORE_DEBUG
        LOG_Trace("%s %s old: %f, new: %f %schanged", name, gstr, ptrValue->float32Value, value.float32Value, changed ? "" : "un");
#endif
        break;
    case evBoolean:
        changed = value.booleanValue != ptrValue->booleanValue;
#if VERBOSE_PROPERTY_STORE_DEBUG
        LOG_Trace("%s %s old: %s, new: %s %schanged", name, gstr, ptrValue->booleanValue ? "true" : "false", value.booleanValue ? "true" : "false", changed ? "" : "un");
#endif
        break;
    case evString:
        changed = strncmp(ptrValue->stringValue, value.stringValue, LSD_STRING_PROPERTY_MAX_LENGTH) != 0;
#if VERBOSE_PROPERTY_STORE_DEBUG
        LOG_Trace("%s %s old: %s, new: %s %schanged", name, gstr, ptrValue->stringValue, value.stringValue, changed ? "" : "un");
#endif
        break;
    case evBlobHandle:
        {
            EnsoPropertyValue_u* othValue = LSD_GetPropertyValuePtrFromGroupDirectly(property, group == DESIRED_GROUP ? REPORTED_GROUP : DESIRED_GROUP);
            MemoryHandle_t newbp = value.memoryHandle;
            MemoryHandle_t oldbp = ptrValue->memoryHandle;
            MemoryHandle_t othbp = othValue->memoryHandle;
            changed = (oldbp && newbp) ? strcmp(oldbp, newbp) != 0 : oldbp != newbp;
#if VERBOSE_PROPERTY_STORE_DEBUG
            LOG_Trace("%s %s old: %08p \"%s\" new: %08p \"%s\" %schanged", name, gstr,
                oldbp, oldbp ? oldbp : "", newbp, newbp ? newbp : "", changed ? "" : "un");
#endif
            if (changed)
            {
                if (oldbp)
                {
                    if (othbp == oldbp)
                    {
                        LOG_Error("Duplicate Blob %p, new %p", oldbp, newbp);
                    } else {
                        OSAL_Free(oldbp);
                    }
                }

                if (newbp)
                {
                    int length = strlen(value.memoryHandle);
                    if (length > 120)
                    {
                        LOG_Warning("*** Exceeding tested length of individual blobs ***");
                    }
                }
            }
            else if (newbp)
            {
                OSAL_Free(newbp);
            }
        }
        break;
    case evTimestamp:
        changed = value.timestamp.seconds != ptrValue->timestamp.seconds ||
            value.timestamp.isValid != ptrValue->timestamp.isValid;
#if VERBOSE_PROPERTY_STORE_DEBUG
        LOG_Trace("%s %s old: %d - %svalid, new: %d - %svalid %schanged", name, gstr,
            ptrValue->timestamp.seconds, ptrValue->timestamp.isValid ? "" : "in",
            value.timestamp.seconds, ptrValue->timestamp.isValid ? "" : "in", changed ? "" : "un");
#endif
        break;
    default:
        LOG_Error("Unknown type %d", property->type.valueType);
        return eecParameterOutOfRange;
    }
    if (changed)
    {
        *ptrValue = value;
        return eecNoError;
    }
    else
    {
        return eecNoChange;
    }
}

/**
 * \name LSD_InitialisePropertyStore
 *
 * NOT THREAD SAFE
 *
 * \brief Used to initialise the EnsoAgent3 property store.
 *
 * NOT THREAD SAFE
 *
 */
EnsoErrorCode_e LSD_InitialisePropertyStore(Mutex_t * mutex)
{
    if (!mutex)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }
    pLSD_Mutex = mutex;
    return prv_InitialisePropertyStore(&prv_PropertyStore);
}

/**
 * \name    LSD_GetPropertyValuePtrFromGroupDirectly
 *
 * NOT THREAD SAFE
 *
 * \brief   Gets the pointer to the property group value of a property.
 *          NOT AT ALL THREAD SAFE
 *
 * \param   property        The property to get.
 *
 * \param   propertyGroup   The property group of to which the value applies
 *
 * \return                  Pointer to the value, it can be directly modified
 *                          and this has clear thread safety implications.
 */
EnsoPropertyValue_u* LSD_GetPropertyValuePtrFromGroupDirectly(
        const EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup )
{
    EnsoPropertyValue_u *valuePtr = NULL;

    if ( NULL != property )
    {
        switch ( propertyGroup )
        {
        case DESIRED_GROUP:
            valuePtr = (EnsoPropertyValue_u* ) &(property->desiredValue);
            break;

        case REPORTED_GROUP:
            valuePtr = (EnsoPropertyValue_u* ) &(property->reportedValue);
            break;

        default:
            break;
        }
    }

    return valuePtr;
}

/**
 * \name    LSD_SetPropertyOutOfSyncDirectly
 *
 * NOT THREAD SAFE
 *
 * \brief   Set the out-of-sync flag for a property group of a property
 *
 * \param   deviceId        The owner that the property belongs to
 *
 * \param   propName        The property to modify
 *
 * \param   propertyGroup   The property group
 *
 * \return                  EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_SetPropertyOutOfSyncDirectly(
        const EnsoDeviceId_t* deviceId,
        const char* propName,
        const PropertyGroup_e propertyGroup)
{
    if (!propName || !deviceId)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    EnsoObject_t* owner = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if (!owner)
    {
        return eecEnsoObjectNotFound;
    }

    EnsoProperty_t* property;
    property = LSD_FindPropertyByCloudSideIdDirectly(owner, propName );
    if (!property)
    {
        return eecPropertyNotFound;
    }

    if (PROPERTY_PRIVATE == property->type.kind)
    {
        return eecPropertyWrongType;
    }

    EnsoErrorCode_e retVal = eecNoError;
    switch (propertyGroup)
    {
        case DESIRED_GROUP:
            property->type.desiredOutOfSync = true;
            // Set the owner as out-of-sync as well.
            owner->desiredOutOfSync = true;
            break;

        case REPORTED_GROUP:
            property->type.reportedOutOfSync = true;
            // Set the owner as out-of-sync as well.
            owner->reportedOutOfSync = true;
            break;

        default:
            assert(0);
            retVal = eecPropertyGroupNotSupported;
            break;
    }

    return retVal;
}


/**
 * \name LSD_CreatePropertyDirectly
 *
 * \brief Used to create a property. By default the property is private.
 *
 * NOT THREAD SAFE.
 *
 * \param   owner                       The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param agentSideId                   The property ID as seen on the agent
 *                                      side and provided by the ensoAgent
 *                                      component, typically the Device
 *                                      Application Layer and will often be
 *                                      based on physical characteristics such
 *                                      as ZigBee cluster and endpoint or
 *                                      Sprue identifiers.
 *
 * \param cloudSideId                   The identifier as seen on the local
 *                                      shadow and consequently used by the
 *                                      thing shadow (ensoCloud) to access the
 *                                      property.
 *
 * \param type                          The property value type
 *
 * \param kind                          The property kind
 *
 * \param buffered                      Buffered or not
 *
 * \param persistent                    Persistent or not
 *
 * \param value                         The property value
 *
 * \return                              A negative number denotes an error
 *                                      code.
 *
 */
EnsoErrorCode_e LSD_CreatePropertyDirectly(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId,
        const EnsoValueType_e type,
        const PropertyKind_e kind,
        const bool buffered,
        const bool persistent,
        const EnsoPropertyValue_u* groupValues)
{
    return prv_CreateProperty(&prv_PropertyStore, owner, agentSideId,
            cloudSideId, type, kind, buffered, persistent, groupValues);
}


/**
 * \name LSD_FindPropertyByAgentSideIdDirectly
 *
 * \brief Used to find a property using the agent (i.e. device) side ID.
 *
 * NOT THREAD SAFE
 *
 * To set the property value using this function it must be a simple type: i.e.
 * evInt32, evUnsignedInt32, evFloat32 or evBoolean.
 *
 * \param owner                         The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param agentSideId                   The property ID as seen on the agent
 *                                      side and provided by the ensoAgent
 *                                      component, typically the Device
 *                                      Application Layer and will often be
 *                                      based on physical characteristics such
 *                                      as ZigBee cluster and endpoint or
 *                                      Sprue identifiers.
 *
 * \return                              The pointer to the property if found
 *                                      or NULL if false.
 *
 */

EnsoProperty_t* LSD_FindPropertyByAgentSideIdDirectly(
        const EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId)
{
    /* Sanity check */
    if (!owner)
    {
        return NULL;
    }

    int propertyIndex;
    EnsoErrorCode_e retVal = prv_GetPropertyIndexByClientSideId(&prv_PropertyStore, owner, agentSideId, &propertyIndex);

    EnsoProperty_t* theProperty = NULL;

    if ((eecNoError == retVal) &&
        (propertyIndex >= 0) &&
        (propertyIndex < LSD_PROPERTY_POOL_SIZE))
    {
        theProperty = &(prv_PropertyStore.propertyPool[propertyIndex].property);
    }

    return theProperty;
}

/**
 * \name LSD_SetPropertyValueByAgentSideIdDirectly
 *
 * \brief Used to set the value of a property.
 *
 * NOT THREAD SAFE
 *
 * To set the property value using this function it must be a simple type: i.e.
 * evInt32, evUnsignedInt32, evFloat32 or evBoolean.
 *
 * \param owner                         The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param agentSideId                   The property ID as seen on the agent side
 *
 * \param newValue                      The new value to set the property to.
 *
 * \return                              A negative number denotes an error
 *                                      code.
 *
 */
EnsoErrorCode_e LSD_SetPropertyValueByAgentSideIdDirectly(
        EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSideId,
        const EnsoPropertyValue_u newValue)
{
#if VERBOSE_PROPERTY_STORE_DEBUG
    LOG_Trace("%s id:%x,", LSD_Group2s(propertyGroup), agentSideId);
#endif
    if (!owner)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    EnsoProperty_t* theProperty = LSD_FindPropertyByAgentSideIdDirectly(owner, agentSideId);
    if (!theProperty)
    {
        return eecPropertyNotFound;
    }

    EnsoErrorCode_e retVal = prv_SetProperty(theProperty, propertyGroup, newValue);

    if (retVal == eecNoError)
    {
        // Only public properties must be synced.
        if (PROPERTY_PUBLIC == theProperty->type.kind)
        {
            if (propertyGroup == DESIRED_GROUP)
            {
                theProperty->type.desiredOutOfSync = true;
                owner->desiredOutOfSync = true;
            }
            else
            {
#if VERBOSE_PROPERTY_STORE_DEBUG
                LOG_Trace("Setting reported Out of Sync");
#endif
                theProperty->type.reportedOutOfSync = true;
                owner->reportedOutOfSync = true;
            }
        }
    }

    return retVal;
}

/*
 * \brief Set the properties of a group within the object.
 *
 * THIS IS NOT THREAD SAFE
 *
 * \param   owner           The identifier of the thing in the ensoAgent
 *
 * \param   propertyGroup   The group to which the properties belong
 *
 * \param   propertyDelta   The block of property agent side identifiers and
 *                          their new values.
 *
 * \param   numProperties   The number of properties in propertyDelta.
 *
 * \return  the number of properties updated
 */
uint16_t LSD_SetPropertiesOfObjectDirectly(
        EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoPropertyDelta_t* propertyDelta,
        const uint16_t numProperties)
{
#if VERBOSE_PROPERTY_STORE_DEBUG
    LOG_Trace("%s no:%d,", LSD_Group2s(propertyGroup), numProperties);
#endif
    if (!owner || !propertyDelta)
    {
        return 0;
    }
    if (propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return 0;
    }

    uint16_t updated = 0;
    for (int i = 0; i < numProperties; i++)
    {
        EnsoErrorCode_e retVal = LSD_SetPropertyValueByAgentSideIdDirectly(owner,
                propertyGroup, propertyDelta[i].agentSidePropertyID, propertyDelta[i].propertyValue);
        if (eecPropertyNotFound == retVal)
        {
            LOG_Warning("Property 0x%X not found", propertyDelta[i].agentSidePropertyID);
            // Continue processing the delta
        }
        else if (eecNoError == retVal)
        {
            ++updated;
        }
        else if (eecNoChange == retVal)
        {
            // Do nothing, we don't need to notify
        }
    }

    return updated;
}


/**
 * \brief   Send properties that meet the filter condition to a destination handler
 *
 * \param   filter          Filter for sending the properties
 *
 * \param   owner           Object to filter
 *
 * \param   destination     Where to send the deltas
 *
 * \param   propertyGroup   Whichever property group to collate the changes
 *                          from

 * \param   maxMessages     Maximum number of messages to send
 *
 * \param[out] numMessages  Number of messages sent
 *
 * \return                  EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_SendPropertiesByFilter_Safe(
        const PropertyFilter_e filter,
        EnsoObject_t* owner,
        const HandlerId_e destination,
        const PropertyGroup_e propertyGroup,
        const uint16_t maxMessages,
        uint16_t* numMessages)
{
    /* Sanity checks */
    if (!owner || !numMessages)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }
    if (( propertyGroup < 0) || ( propertyGroup >= PROPERTY_GROUP_MAX))
    {
        return eecPropertyGroupNotSupported;
    }

    // Initialise the return parameter
    *numMessages = 0;

    if (PROPERTY_FILTER_OUT_OF_SYNC == filter)
    {
        // Check that there is something to do
        if ((DESIRED_GROUP == propertyGroup && !owner->desiredOutOfSync) ||
            (REPORTED_GROUP == propertyGroup && !owner->reportedOutOfSync))
        {
            // Nothing to do
            return eecNoError;
        }
    }

    EnsoErrorCode_e retVal = eecNoError;

    // Lock Local Shadow
    OSAL_LockMutex(pLSD_Mutex);

    EnsoPropertyDelta_t propertyDelta[ECOM_MAX_DELTAS];
    unsigned int currentDelta = 0; // Current property delta index in the ECOM message

    if (LSD_IsEnsoDeviceIdValid(owner->deviceId))
    {
        int i = owner->propertyListStart;

        while ((*numMessages < maxMessages) && (i >= 0))
        {
            // Set the property condition based on the filter
            bool propCond = false;
            switch (filter)
            {
                case PROPERTY_FILTER_OUT_OF_SYNC:
                    if (REPORTED_GROUP == propertyGroup)
                    {
                        propCond = prv_PropertyStore.propertyPool[i].property.type.reportedOutOfSync;
                    }
                    else
                    {
                        propCond = prv_PropertyStore.propertyPool[i].property.type.desiredOutOfSync;
                    }
                    break;
                case PROPERTY_FILTER_PERSISTENT:
                    propCond = prv_PropertyStore.propertyPool[i].property.type.persistent;
                    break;
                case PROPERTY_FILTER_TIMESTAMPS:
                    propCond = prv_PropertyStore.propertyPool[i].property.type.valueType == evTimestamp;
                    break;
                case PROPERTY_FILTER_ALL:
                    propCond = true;
                    break;
                default:
                    break;
            }

            // Check the property
            switch (propertyGroup)
            {
                case REPORTED_GROUP:
                    if (propCond)
                    {
                        propertyDelta[currentDelta].agentSidePropertyID = prv_PropertyStore.propertyPool[i].property.agentSidePropertyID;
                        propertyDelta[currentDelta].propertyValue = prv_PropertyStore.propertyPool[i].property.reportedValue;
                        if (PROPERTY_FILTER_OUT_OF_SYNC == filter)
                        {
                            // Only feedback from cloud can clear this flag
                            //prv_PropertyStore.propertyPool[i].property.type.reportedOutOfSync = false;
                        }
                        currentDelta++;
                    }
                    break;

                case DESIRED_GROUP:
                    if (propCond)
                    {
                        propertyDelta[currentDelta].agentSidePropertyID = prv_PropertyStore.propertyPool[i].property.agentSidePropertyID;
                        propertyDelta[currentDelta].propertyValue = prv_PropertyStore.propertyPool[i].property.desiredValue;
                        if (PROPERTY_FILTER_OUT_OF_SYNC == filter)
                        {
                            // We can send desired at start up if we have them marked as out of sync
                            // but desired properties are not our responsibility. As soon as they
                            // are returned to us we will set the shadow to whatever we receive
                            prv_PropertyStore.propertyPool[i].property.type.desiredOutOfSync = false;
                        }
                        currentDelta++;
                    }
                    break;

                default:
                    break;
            }
            i = prv_PropertyStore.propertyPool[i].nextContainerIndex;

            // Send an update message to the destination handler
            if (currentDelta >= ECOM_MAX_DELTAS)
            {
                LOG_Trace("Sending UPDATE message with %d deltas", currentDelta);
                retVal = ECOM_SendUpdateToSubscriber(
                    destination,
                    owner->deviceId,
                    propertyGroup,
                    currentDelta, // Number of properties in the delta
                    propertyDelta);
                if (eecNoError != retVal)
                {
                    LOG_Error("Error sending message to %d", destination);
                }

                currentDelta = 0;
                *numMessages += 1;
            }
        }

        // Go through list of properties to see if owner is in sync now
        int j = owner->propertyListStart;
        if (PROPERTY_FILTER_OUT_OF_SYNC == filter)
        {
            // Check if the object is now in sync
            switch (propertyGroup)
            {
                case REPORTED_GROUP:
                    owner->reportedOutOfSync = false;

                    while (j >= 0)
                    {
                        if (prv_PropertyStore.propertyPool[j].property.type.reportedOutOfSync)
                        {
                            owner->reportedOutOfSync = true;
                            break;
                        }
                        j = prv_PropertyStore.propertyPool[j].nextContainerIndex;
                    }
                    break;

                case DESIRED_GROUP:
                    owner->desiredOutOfSync = false;
                    // Start i where we left of
                    while (j >= 0)
                    {
                        if (prv_PropertyStore.propertyPool[j].property.type.desiredOutOfSync)
                        {
                            owner->desiredOutOfSync = true;
                            break;
                        }
                        j = prv_PropertyStore.propertyPool[j].nextContainerIndex;
                    }
                    break;

                default:
                    break;
            }
        }
    } // End of LSD_IsEnsoDeviceIdValid

    // Unlock local shadow
    OSAL_UnLockMutex(pLSD_Mutex);

    if (currentDelta)
    {
        // Send the delta to the destination handler
        LOG_Trace("Sending UPDATE message with %d deltas", currentDelta);
        retVal = ECOM_SendUpdateToSubscriber(
                destination,
                owner->deviceId,
                propertyGroup,
                currentDelta, // Number of properties in the delta
                propertyDelta);
        if (eecNoError != retVal)
        {
            LOG_Error("Error sending message to %d", destination);
        }
        *numMessages += 1;
    }

    return retVal;
}

/**
 * \name LSD_FindPropertyByCloudSideIdDirectly
 *
 * \brief Used to find a property using the agent (i.e. device) side ID.
 *
 * NOT THREAD SAFE
 *
 * To set the property value using this function it must be a simple type: i.e.
 * evInt32, evUnsignedInt32, evFloat32 or evBoolean.
 *
 * \param owner                         The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param cloudSideId                   The identifier as seen on the local
 *                                      shadow and consequently used by the
 *                                      thing shadow (ensoCloud) to access the
 *                                      property.
 *
 * \return                              The pointer to the property if found
 *                                      or NULL if false.
 *
 */

EnsoProperty_t* LSD_FindPropertyByCloudSideIdDirectly(
        const EnsoObject_t* owner,
        const char* cloudSideId)
{
    /* Sanity checks */
    if ( ( NULL == owner ) || ( NULL == cloudSideId ) )
    {
        return NULL;
    }

    EnsoProperty_t* theProperty = NULL;
    int propertyIndex;
    EnsoErrorCode_e retVal = prv_GetPropertyIndexByCloudName(&prv_PropertyStore, owner, cloudSideId, &propertyIndex);

    if ((eecNoError == retVal) &&
        (propertyIndex >= 0) &&
        (propertyIndex < LSD_PROPERTY_POOL_SIZE))
    {
        theProperty = &(prv_PropertyStore.propertyPool[propertyIndex].property);
    }

    return theProperty;
}


/**
 * \name LSD_CopyBufferToPropertyDirectly
 *
 * \brief Used to copy the buffer to memory block associated with the property
 *
 * NOT THREAD SAFE
 *
 * The buffer containing the value is copied to a memory block associated with
 * the property of the ensoObject.
 *
 * \param property                      The property to modify
 *
 * \param propertyGroup                 Whichever property group is to be
 *                                      updated.
 *
 * \param bufferSize                    The size of the contents to copy to
 *                                      the property.
 *
 * \param newBuffer                     The new content to store in the
 *                                      property.
 *
 * \return                              A negative number denotes an error
 *                                      code.
 *
 */
EnsoErrorCode_e LSD_CopyBufferToPropertyDirectly(
        EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup,
        const size_t bufferSize,
        const void* newBuffer)
{
    // Sanity checks
    if (!property || !newBuffer)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }
    if (!bufferSize)
    {
        // Nothing to do
        return eecNoError;
    }

    EnsoPropertyValue_u* value = LSD_GetPropertyValuePtrFromGroupDirectly(property, propertyGroup);
    if (!value)
    {
        // We really shouldn't get here and because we've already checked the
        // property it must be the group.
        return eecPropertyGroupNotSupported;
    }

    EnsoErrorCode_e retVal = eecNoError;
    switch (property->type.valueType)
    {
        case evBlobHandle:
        {
            bool changed = (value->memoryHandle && newBuffer) ?
                    strncmp(value->memoryHandle, newBuffer, bufferSize) != 0 : true;

            if (changed)
            {
                // Get new memory for this change
                EnsoPropertyValue_u newValue;
                newValue.memoryHandle = OSAL_MemoryRequest((MemoryPoolHandle_t) NULL, bufferSize);

                if (newValue.memoryHandle)
                {
                    memcpy(newValue.memoryHandle, newBuffer, bufferSize);
                    retVal = prv_SetProperty(property, propertyGroup, newValue);
                }
                else
                {
                    retVal = eecPoolFull;
                }
            }
            else
            {
                retVal = eecNoChange;
            }
            break;
        }

        default:
            LOG_Error("Expected a blob handle");
            retVal = eecPropertyValueNotObjectHandle;
            break;
    }

    return retVal;
}


/**
 * \name    LSD_CopyFromPropertyBufferByAgentSideIdDirectly
 *
 * \brief   If a property contains a string or other form of memory store
 *          this function copies its content into the user supplied buffer,
 *          otherwise it copies nothing and returns an error.
 *
 * NOT THREAD SAFE
 *
 * \param owner                         The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param   propertyGroup               Whichever property group is to be
 *                                      copied from.
 *
 * \param agentSideId                   The property ID as seen on the agent
 *                                      side and provided by the ensoAgent
 *                                      component, typically the Device
 *                                      Application Layer and will often be
 *                                      based on physical characteristics such
 *                                      as ZigBee cluster and endpoint or
 *                                      Sprue identifiers.
 *
 * \param bufferSize                    The size of the buffer supplied by the
 *                                      user to receive the contents of the
 *                                      property.
 *
 * \param pRxBuffer                     Pointer to the buffer supplied by the
 *                                      user to be filled in with the contents
 *                                      of the property. The buffer must be
 *                                      big enough to receive the content of
 *                                      the property.
 *
 * \param pBytesCopied                  A pointer to an variable that will be
 *                                      filled in with the number of bytes
 *                                      copied.
 *
 * \return                              A negative number denotes an error
 *                                      code.
 */
EnsoErrorCode_e prv_CopyFromPropertyBufferDirectly(
        const EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup,
        const size_t bufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied)
{
    if ( NULL == property)
    {
        return eecPropertyNotFound;
    }

    EnsoPropertyValue_u* value = LSD_GetPropertyValuePtrFromGroupDirectly ( property, propertyGroup );
    if ( NULL == value )
    {
        // We really shouldn't get here and because we've already checked the
        // property it must be the group.
        return eecPropertyGroupNotSupported;
    }

    EnsoErrorCode_e retVal = eecNoError;
    switch ( property->type.valueType )
    {
    case evBlobHandle:
        {
            size_t size = value->memoryHandle ? OSAL_GetAmountStored(value->memoryHandle) : 0;
            if (bufferSize >= size )
            {
                memcpy(pRxBuffer, value->memoryHandle, size );
                *pBytesCopied = size;
            }
            else
            {
                retVal = eecBufferTooSmall;
                *pBytesCopied = 0;
            }
        }
        break;

    default:
        retVal = eecPropertyValueNotObjectHandle;
        break;
    }

    return retVal;
}


/**
 * \name    LSD_CopyFromPropertyBufferByAgentSideIdDirectly
 *
 * \brief   If a property contains a string or other form of memory store
 *          this function copies its content into the user supplied buffer,
 *          otherwise it copies nothing and returns an error.
 *
 * NOT THREAD SAFE
 *
 * \param owner                         The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param   propertyGroup               Whichever property group is to be
 *                                      copied from.
 *
 * \param agentSideId                   The property ID as seen on the agent
 *                                      side and provided by the ensoAgent
 *                                      component, typically the Device
 *                                      Application Layer and will often be
 *                                      based on physical characteristics such
 *                                      as ZigBee cluster and endpoint or
 *                                      Sprue identifiers.
 *
 * \param bufferSize                    The size of the buffer supplied by the
 *                                      user to receive the contents of the
 *                                      property.
 *
 * \param pRxBuffer                     Pointer to the buffer supplied by the
 *                                      user to be filled in with the contents
 *                                      of the property. The buffer must be
 *                                      big enough to receive the content of
 *                                      the property.
 *
 * \param pBytesCopied                  A pointer to an variable that will be
 *                                      filled in with the number of bytes
 *                                      copied.
 *
 * \return                              A negative number denotes an error
 *                                      code.
 */
EnsoErrorCode_e LSD_CopyFromPropertyBufferByAgentSideIdDirectly(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSideId,
        const size_t bufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied)
{
    EnsoProperty_t* property = LSD_FindPropertyByAgentSideIdDirectly( owner, agentSideId );
    if ( NULL == property)
    {
        return eecPropertyNotFound;
    }

    EnsoErrorCode_e retVal = prv_CopyFromPropertyBufferDirectly(
            property,
             propertyGroup,
            bufferSize,
            pRxBuffer,
            pBytesCopied);

    return retVal;
}


/**
 * \name    LSD_GetSizeOfPropertyBufferDirectly
 *
 * \brief   Get the size of the buffer associated with the property, this must
 *          be >= the amount stored in the buffer (obviously).
 *
 * NOT THREAD SAFE
 *
 * \param owner                         The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param propertyGroup                 Whichever property group is to be
 *                                      examined
 *
 * \param bufferSize                    Pointer to a variable that receives
 *                                      the size of the buffer associated with
 *                                      that property.
 *
 * \return                              A negative number denotes an error
 *                                      code.
 */
EnsoErrorCode_e LSD_GetSizeOfPropertyBufferDirectly( const EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup,
        size_t* bufferSize)
{
    if ( ( NULL == property) || ( NULL == bufferSize) )
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    EnsoPropertyValue_u* value = LSD_GetPropertyValuePtrFromGroupDirectly ( property, propertyGroup );
    if ( NULL == value )
    {
        // We really shouldn't get here and because we've already checked the
        // property it must be the group.
        return eecPropertyGroupNotSupported;
    }

    EnsoErrorCode_e retVal = eecNoError;
    switch ( property->type.valueType )
    {
    case evBlobHandle:
            *bufferSize = value->memoryHandle ? OSAL_GetBlockSize(value->memoryHandle) : 0;
        break;

    default:
        retVal = eecPropertyValueNotObjectHandle;
        break;
    }

    return retVal;
}


/**
 * \name    LSD_GetSizeOfPropertyBufferContentDirectly
 *
 * \brief   Get the amount of content in the buffer.
 *
 * NOT THREAD SAFE
 *
 * \param owner                         The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param propertyGroup                 Whichever property group is to be
 *                                      examined
 *
 * \param bufferSize                    Pointer to a variable that receives
 *                                      the size of the buffer associated with
 *                                      that property.
 *
 * \return                              A negative number denotes an error
 *                                      code.
 */
EnsoErrorCode_e LSD_GetSizeOfPropertyBufferContentDirectly(
        const EnsoProperty_t* property,
        const PropertyGroup_e propertyGroup,
        size_t* bufferSize)
{
    if ( ( NULL == property) || ( NULL == bufferSize) )
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    EnsoPropertyValue_u* value = LSD_GetPropertyValuePtrFromGroupDirectly ( property, propertyGroup );
    if ( NULL == value )
    {
        // We really shouldn't get here and because we've already checked the
        // property it must be the group.
        return eecPropertyGroupNotSupported;
    }

    EnsoErrorCode_e retVal = eecNoError;
    switch ( property->type.valueType )
    {
    case evBlobHandle:
            *bufferSize = value->memoryHandle ? OSAL_GetAmountStored(value->memoryHandle) : 0;
        break;

    default:
        retVal = eecPropertyValueNotObjectHandle;
        break;
    }

    return retVal;
}


/**
 * \name LSD_GetPropertyValueByCloudSideIdDirectly
 *
 *
 * NOT THREAD SAFE
 *
 *
 */
EnsoErrorCode_e LSD_GetPropertyValueByCloudSideIdDirectly(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const char* cloudSideId,
        EnsoPropertyValue_u* pRxValue)
{
    if ( ( NULL == owner ) || ( NULL == cloudSideId ) || ( NULL == pRxValue ) )
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }
    if ( ( propertyGroup < 0 ) || ( propertyGroup >= PROPERTY_GROUP_MAX ) )
    {
        return eecPropertyGroupNotSupported;
    }

    EnsoErrorCode_e retVal = eecPropertyNotFound;
    EnsoProperty_t* theProperty = LSD_FindPropertyByCloudSideIdDirectly( owner, cloudSideId );
    if ( NULL != theProperty )
    {
        EnsoPropertyValue_u* ptrValue = LSD_GetPropertyValuePtrFromGroupDirectly(theProperty, propertyGroup );
        if ( NULL != ptrValue )
        {
            *pRxValue = *ptrValue;
            retVal = eecNoError;
        }
    }

    return retVal;
}


/**
 * \name    LSD_CopyFromPropertyBufferByCloudSideIdDirectly
 *
 * \brief   If a property contains a string or other form of memory store
 *          this function copies its content into the user supplied buffer,
 *          otherwise it copies nothing and returns an error.
 *
 * NOT THREAD SAFE
 *
 * \param owner                         The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param propertyGroup                 Whichever property group is to be
 *                                      copied from.
 *
 * \param cloudSideId                   The property ID as seen on the
 *                                      ensCloud/local shadow side.
 *
 * \param bufferSize                    The size of the buffer supplied by the
 *                                      user to receive the contents of the
 *                                      property.
 *
 * \param pRxBuffer                     Pointer to the buffer supplied by the
 *                                      user to be filled in with the contents
 *                                      of the property. The buffer must be
 *                                      big enough to receive the content of
 *                                      the property.
 *
 * \param pBytesCopied                  A pointer to an variable that will be
 *                                      filled in with the number of bytes
 *                                      copied.
 *
 * \return                              A negative number denotes an error
 *                                      code.
 */
EnsoErrorCode_e LSD_CopyFromPropertyBufferByCloudSideIdDirectly(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const char* cloudSideId,
        const size_t bufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied)
{
    /* Sanity Checks */
    if ( ( NULL == owner )        || ( NULL == cloudSideId ) ||
         ( NULL == pBytesCopied ) || ( NULL == pRxBuffer) )
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    if ( propertyGroup <0  ||  propertyGroup >= PROPERTY_GROUP_MAX )
    {
        return eecPropertyGroupNotSupported;
    }

    if ( 0 == bufferSize)
    {
        return eecBufferTooSmall;
    }

    EnsoProperty_t* property = LSD_FindPropertyByCloudSideIdDirectly( owner, cloudSideId );
    if ( NULL == property)
    {
        return eecPropertyNotFound;
    }

    EnsoErrorCode_e retVal = prv_CopyFromPropertyBufferDirectly(
            property,
             propertyGroup,
            bufferSize,
            pRxBuffer,
            pBytesCopied);

    return retVal;
}

/**
 * \name    LSD_IsPropertyNested
 *
 * \brief   Whether a property is a nested property i.e. its representation
 *          in AWS Shadow is JSON object with one level of nesting. Its
 *          representation in Local Shadow is of the form "parent_child".
 *
 * \param cloudName                  The property cloud name stored in Local Shadow
 *
 * \param parentName                 For a nested property, the parent name.
 *
 * \param childName                  For a nested property, the child name..
 *
 * \return                           True for a nested property.
 */
bool LSD_IsPropertyNested(
        const char* cloudName,
        char* parentName,
        char* childName)
{
    /* Sanity Checks */
    if (!cloudName || !parentName || !childName)
    {
        LOG_Error("Null argument");
        return false;
    }

    int parsed = sscanf(cloudName, "%5[^'_']_%5[^'_']_", parentName, childName);
    if (parsed == 2)
    {
 //       LOG_Trace("Nested property: %s %s", parentName, childName);
        return true;
    }
    return false;
}

/**
 * \brief            returns a character representation of a device handler id
 * \param id         Device Handler ID
 */
static char * Handler2s(HandlerId_e id)
{
    switch (id)
    {
    case INVALID_HANDLER:           return "I";
    case COMMS_HANDLER:             return "C";
    case UPGRADE_HANDLER:           return "U";
    case STORAGE_HANDLER:           return "S";
    case AUTOMATION_ENGINE_HANDLER: return "A";
    case LOG_HANDLER:               return "L";
    case TIMESTAMP_HANDLER:         return "T";
    case LSD_HANDLER:               return "s";
    case GW_HANDLER:                return "G";
    case LED_DEVICE_HANDLER:        return "E";
    case WISAFE_DEVICE_HANDLER:     return "W";
    case TEST_DEVICE_HANDLER:       return "t";
    case TELEGESIS_DEVICE_HANDLER:  return "Z";
    default:                        return "u";
    }
}

/**
 * \brief            Dumps an EnsoProperty * to log.
 * \param p          pointer to an EnsoProperty_t
 */
void LSD_DumpProperty(EnsoProperty_t * p)
{
    char desi[12];
    char desisub[128] = "";
    int desioffs = 0;
    char repo[12];
    char reposub[128] = "";
    int repooffs = 0;
    for (int i = 0; i < 32; i++)
    {
        uint32_t bit = 1 << i;
        if (p->subscriptionBitmap[DESIRED_GROUP] & bit)
        {
            HandlerId_e id = LSD_SubscriberArray[DESIRED_GROUP][i].handlerId;
            bool priv = LSD_SubscriberArray[DESIRED_GROUP][i].isPrivate;
            desioffs += snprintf(&desisub[desioffs], sizeof desisub - desioffs, " %s%s", Handler2s(id), priv ? "p" : "-");
        }
        if (p->subscriptionBitmap[REPORTED_GROUP] & bit)
        {
            HandlerId_e id = LSD_SubscriberArray[REPORTED_GROUP][i].handlerId;
            bool priv = LSD_SubscriberArray[REPORTED_GROUP][i].isPrivate;
            repooffs += snprintf(&reposub[repooffs], sizeof reposub - repooffs, " %s%s", Handler2s(id), priv ? "p" : "-");
        }
    }
    EnsoValueType_e type = p->type.valueType;
    LOG_Info("%-11.11s %08x %s %s %s %s des:%s %-64.64s rep:%s %-64.64s des:%08x %-12.12s rep: %08x %-12.12s",
        p->cloudName,
        p->agentSidePropertyID,
        p->type.persistent      ? "Prs" : "   ",
        p->type.kind == PROPERTY_PRIVATE ? "Pri" :
        p->type.kind == PROPERTY_PUBLIC  ? "Pub" : "???",
        LSD_ValueType2s(type),
        p->type.buffered        ? "Buf" : "   ",
        p->type.desiredOutOfSync ? "?" : " ",
        LSD_Value2s(desi, sizeof desi, type, &p->desiredValue),
        p->type.reportedOutOfSync ? "?" : " ",
        LSD_Value2s(repo, sizeof repo, type, &p->reportedValue),
        p->subscriptionBitmap[DESIRED_GROUP], desisub,
        p->subscriptionBitmap[REPORTED_GROUP], reposub
    );
}

/**
 * \brief            Dumps the property list staring at index
 * \param index      index in propertyPool of first property.
 */
void LSD_DumpProperties(int index)
{
    OSAL_LockMutex(pLSD_Mutex);
    for (int i = index; i >= 0; i = prv_PropertyStore.propertyPool[i].nextContainerIndex)
    {
        LSD_DumpProperty(&prv_PropertyStore.propertyPool[i].property);
    }
    OSAL_UnLockMutex(pLSD_Mutex);
}
