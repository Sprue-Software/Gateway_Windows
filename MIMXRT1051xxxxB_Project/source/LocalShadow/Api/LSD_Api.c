/*!****************************************************************************
 *
 * \file LSD_API.c
 *
 * \author Carl Pickering
 *
 * \brief Implementation of interface to the local shadow.
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stddef.h>
#include <string.h>

#include "OSAL_Api.h"
#include "LSD_Types.h"
#include "LSD_EnsoObjectStore.h"
#include "LSD_PropertyStore.h"
#include "LSD_Api.h"
#include "LSD_Subscribe.h"
#include "LOG_Api.h"
#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "APP_Types.h"
#include "SYS_Gateway.h"



/*!****************************************************************************
 * Static variables
 *****************************************************************************/

static Mutex_t lsdMutex;


/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

static EnsoErrorCode_e prv_NotifyStorageOfPersistentProperty(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId);

static void _LSD_MessageQueueListener(MessageQueue_t mq);


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/*
 * \brief Initialise the Local Shadow
 *
 */
EnsoErrorCode_e LSD_Init(void)
{
    LOG_Info("LSD_Init");

    OSAL_InitMutex(&lsdMutex, NULL);
    EnsoErrorCode_e  retVal = LSD_EnsoObjectStoreInit();
    if ( eecNoError == retVal )
    {
        retVal = LSD_InitialisePropertyStore(&lsdMutex);
        if (eecNoError == retVal)
        {
            LSD_SubscribeInit();
        }
    }

    const char * name = "/lsdH";
    MessageQueue_t mq = OSAL_NewMessageQueue(name, ECOM_MAX_MESSAGE_QUEUE_DEPTH,
                                             ECOM_MAX_MESSAGE_SIZE);
    if (mq == NULL)
    {
        LOG_Error("OSAL_NewMessageQueue(\"%s\", %d, %d) failed", name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
        return eecInternalError;
    }

    // Create listening thread
    Thread_t listeningThread = OSAL_NewThread(_LSD_MessageQueueListener, mq);
    if (listeningThread == NULL)
    {
        LOG_Error("OSAL_NewThread failed for stoH");
        return eecInternalError;
    }

    ECOM_RegisterMessageQueue(LSD_HANDLER, mq);

    return retVal;
}


/*
 * \brief Helper function to get time to set value for a timestamp
 *
 * \return     property value to use in a timestamp
 */
EnsoPropertyValue_u LSD_GetTimeNow() {

    EnsoPropertyValue_u newValue;

    newValue.timestamp.seconds = OSAL_GetTimeInSecondsSinceEpoch();

    // Time defaults to Jan 2000 if we don't have actual time yet
    newValue.timestamp.isValid = (newValue.timestamp.seconds > 978307200); // Jan 1, 2001

    return newValue;
}


/*****************************************************************************/
/*                                                                           */
/* Object based functions                                                    */
/*                                                                           */
/*****************************************************************************/

/**
 * \name LSD_CreateEnsoObject
 *
 * \brief Initialise a new ensoObject
 *
 * This function is used to create an ensoObject.
 *
 * \param newId             The unique thing identifier
 *
 * \return EnsoObject_t*    Pointer to the new ensoObject
 *
 */
EnsoObject_t* LSD_CreateEnsoObject(const EnsoDeviceId_t newId)
{
    OSAL_LockMutex(&lsdMutex);
    EnsoObject_t* newObject = LSD_CreateEnsoObjectDirectly(newId);
    OSAL_UnLockMutex(&lsdMutex);

    return newObject;
}


/**
 * LSD_DestroyEnsoDevice
 *
 * \brief Destroy the device and its properties
 *
 * \param deviceId      Id of the thing to destroy.
 *
 * \return              EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_DestroyEnsoDevice(const EnsoDeviceId_t deviceId)
{
    if (!LSD_IsEnsoDeviceIdValid(deviceId))
    {
        return eecEnsoDeviceNotFound;
    }

    EnsoErrorCode_e retVal = LSD_RemoveAllPropertiesOfObject_Safe(&deviceId);
    if (eecNoError == retVal)
    {
        OSAL_LockMutex(&lsdMutex);
        LSD_DestroyEnsoDeviceDirectly(deviceId);
        OSAL_UnLockMutex(&lsdMutex);

        // Directly send a delta message to Comms Handler and Storage Handler.
        retVal = ECOM_SendThingStatusToSubscriber(
                STORAGE_HANDLER,
                deviceId,
                THING_DELETED);
        if (eecNoError != retVal)
        {
            LOG_Error("ECOM_SendThingStatusToSubscriber failed %s", LSD_EnsoErrorCode_eToString(retVal));
        }

        retVal = ECOM_SendThingStatusToSubscriber(
                COMMS_HANDLER,
                deviceId,
                THING_DELETED);
        if (eecNoError != retVal)
        {
            LOG_Error("ECOM_SendThingStatusToSubscriber failed %s", LSD_EnsoErrorCode_eToString(retVal));
        }
    }

    return retVal;
}


/**
 * \name LSD_RegisterEnsoObject
 *
 * \brief Register an ensoObject with comms handler and cloud.
 *
 * \param newObject     Pointer to the new object to be registered.
 *
 * \return              EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_RegisterEnsoObject(EnsoObject_t* newObject)
{
    EnsoErrorCode_e retVal = eecNullPointerSupplied;

    if (newObject)
    {
        OSAL_LockMutex(&lsdMutex);
        retVal = LSD_RegisterEnsoObjectDirectly( newObject );
        OSAL_UnLockMutex(&lsdMutex);

        if (eecNoError == retVal)
        {
            // Directly send a delta message to Comms Handler (device status is
            // a private property, so Comms Handler will not get notified via the normal
            // subscription mechanism)
            retVal = ECOM_SendThingStatusToSubscriber(
                    COMMS_HANDLER,
                    newObject->deviceId,
                    THING_DISCOVERED);
            if (eecNoError != retVal)
            {
                LOG_Error("ECOM_SendThingStatusToSubscriber failed %s", LSD_EnsoErrorCode_eToString(retVal));
            }
        }
    }

    return retVal;
}


/**
 * \name LSD_AnnounceEnsoObjects
 *
 * \brief Search for enso objects that are not announced to cloud and annouce them.
 *
 * \param finished      Indicate to caller whether we are done.
 *
 * \return              EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_AnnounceEnsoObjects(bool* finished)
{
    // Walk devices to see if any need announcing
    OSAL_LockMutex(&lsdMutex);
    EnsoObject_t* object = LSD_FindEnsoObjectNeedingAnnounceDirectly();
    OSAL_UnLockMutex(&lsdMutex);


    EnsoErrorCode_e retVal = eecNoError;

    if (object)
    {
        retVal = ECOM_SendThingStatusToSubscriber(
                        COMMS_HANDLER,
                        object->deviceId,
                        THING_DISCOVERED);
        *finished = false;
    }
    else
    {
        // No objects found needing announcing
        *finished = true;
    }

    return retVal;
}




/**
 * \name LSD_CreateProperty
 *
 * \brief Used to create a property.
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
EnsoErrorCode_e LSD_CreateProperty(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId,
        const EnsoValueType_e type,
        const PropertyKind_e kind,
        const bool buffered,
        const bool persistent,
        const EnsoPropertyValue_u* groupValues)
{
    /* Sanity checks */
    if (!owner || !cloudSideId || !groupValues)
    {
        return eecNullPointerSupplied;
    }
    if (0 == agentSideId)
    {
        return eecParameterOutOfRange;
    }
    if (0 == *cloudSideId)
    {
        return eecParameterOutOfRange;
    }
    if (type < evInt32 || type >= evNumObjectTypes)
    {
        return eecParameterOutOfRange;
    }
    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal =  LSD_CreatePropertyDirectly(owner, agentSideId, cloudSideId,
            type, kind, buffered, persistent, groupValues);
    OSAL_UnLockMutex(&lsdMutex);
    if (retVal == eecNoError)
    {
        if (persistent)
        {
            retVal = prv_NotifyStorageOfPersistentProperty(owner, agentSideId);

            if (retVal < 0)
            {
                LOG_Error("Failed to setup storage for persistent property? Err = %s",
                        LSD_EnsoErrorCode_eToString(retVal));
            }
        }
    }

    return retVal;
}



/**
 * \name LSD_CreateProperties
 *
 * \brief Used to create a set of properties atomically.
 *
 * \param   owner                       The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param agentSideId                   Array of property IDs
 *
 * \param cloudSideId                   Array of cloud IDs
 *
 * \param type                          Array of types
 *
 * \param kind                          Is the group public or private
 *
 * \param buffered                      Is the group buffered
 *
 * \param persistent                    Is the group persistent

 * \param value                         Array of values
 *
 * \return                              A negative number denotes an error
 *                                      code.
 */
EnsoErrorCode_e LSD_CreateProperties(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId[],
        const char* cloudSideId[],
        const EnsoValueType_e type[],
        const PropertyKind_e kind,
        const bool buffered,
        const bool persistent,
        EnsoPropertyValue_u groupValues[][PROPERTY_GROUP_MAX],
        uint32_t numProperties)
{
    /* Sanity checks */
    if (!owner || !agentSideId || !cloudSideId || !type || !groupValues)
    {
        return eecNullPointerSupplied;
    }

    EnsoErrorCode_e retVal;

    OSAL_LockMutex(&lsdMutex);
    for (int i=0; i <numProperties; i++)
    {
        retVal = LSD_CreatePropertyDirectly(owner, agentSideId[i],
                cloudSideId[i], type[i], kind, buffered, persistent, groupValues[i]);

        if (retVal != eecNoError)
            break; // bail early
    }
    OSAL_UnLockMutex(&lsdMutex);

    if ((retVal == eecNoError) && (persistent))
    {
        for (int i=0; i <numProperties; i++)
        {
            retVal = prv_NotifyStorageOfPersistentProperty(owner, agentSideId[i]);

            if (retVal != eecNoError)
                break; // bail early
        }
    }

    if ((retVal == eecNoError) && (kind != PROPERTY_PRIVATE))
    {
        // Notify interested parties of the new properties, we only need notify for reported
        int propertyIndex = 0;

        EnsoPropertyDelta_t delta[ECOM_MAX_DELTAS];

        while (propertyIndex < numProperties)
        {
            int deltaIndex = 0;

            while ((propertyIndex < numProperties) && (deltaIndex < ECOM_MAX_DELTAS))
            {
                delta[deltaIndex].propertyValue = groupValues[propertyIndex][REPORTED_GROUP];
                delta[deltaIndex].agentSidePropertyID = agentSideId[propertyIndex];
                deltaIndex++;
                propertyIndex++;
            }


            // We have created a set of properties, need to notify comms handler
            // to send them to cloud if required
            if (deltaIndex > 0)
            {
                EnsoObject_t* destination = LSD_FindEnsoObjectByDeviceIdDirectly(&owner->deviceId);
                retVal = LSD_NotifyDirectly(LSD_HANDLER, destination, REPORTED_GROUP, delta, deltaIndex);
            }
        }
    }

    return retVal;
}



/**
 * \name LSD_RestoreProperty
 *
 * \brief Restore a property. This is called at start-up by Storage Manager to
 *        restore a property from persistent storage.
 *
 * \param   owner                       The ensoObject that shall "own" this
 *                                      property and receive updates when it
 *                                      is changed.
 *
 * \param agentSideId                   The property ID as seen on the agent
 *                                      side and provided by the ensoAgent
 *                                      component.
 *
 * \param cloudSideId                   The identifier as seen on the local
 *                                      shadow and consequently used by the
 *                                      thing shadow (ensoCloud) to access the
 *                                      property.
 *
 * \param propType                      The property type
 *
 * \param value                         The property value
 *
 * \return                              A negative number denotes an error
 *                                      code.
 *
 */
EnsoErrorCode_e LSD_RestoreProperty(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId,
        const char* cloudSideId,
        EnsoPropertyType_t propType,
        const EnsoPropertyValue_u* groupValues)
{
    /* Sanity checks */
    if (!owner || !groupValues || !cloudSideId)
    {
        return eecNullPointerSupplied;
    }
    if (0 == agentSideId)
    {
        return eecParameterOutOfRange;
    }

    // No need to lock the local shadow
    // This code is only called at start-up

    EnsoErrorCode_e retVal =  LSD_CreatePropertyDirectly(owner, agentSideId, cloudSideId,
            propType.valueType, propType.kind, propType.buffered, propType.persistent,
            groupValues);

    // Subscribe storage handler to the property (we know it is persistent)
    retVal = LSD_SubscribeToDevicePropertyByAgentSideIdDirectly(&owner->deviceId,
            agentSideId, DESIRED_GROUP, STORAGE_HANDLER, true);
    if (retVal < 0)
    {
        LOG_Error("Failed to subscribe storage handler to %x desired", agentSideId);
    }

    retVal = LSD_SubscribeToDevicePropertyByAgentSideIdDirectly(&owner->deviceId,
            agentSideId, REPORTED_GROUP, STORAGE_HANDLER, true);

    if (retVal < 0)
    {
        LOG_Error("Failed to subscribe storage handler to %x reported", agentSideId);
    }

    return retVal;
}


/**
 * \name    LSD_RemovePropertyByAgentSideId
 *
 * \brief   This function removes a property of the object and returns it
 *          to the property store.
 *
 * \param   deviceId        The device that owns the property to be removed
 *
 * \param   agentSideId The internal ID of the property to be removed
 *
 * \return  EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_RemovePropertyByAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSideId)
{
    if (!deviceId)
    {
        return eecNullPointerSupplied;
    }
    if (0 == agentSideId)
    {
        return eecPropertyNotFound;
    }

    return LSD_RemoveProperty_Safe(deviceId, false, agentSideId, 0, true);
}

/**
 * \name    LSD_RemovePropertyByCloudSideId
 *
 * \brief   This function removes a property of the object and returns it
 *          to the property store.
 *
 * \param   deviceId    The device that owns the property to be removed
 *
 * \param   cloudName   The name of the property to be removed
 *
 * \return  EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_RemovePropertyByCloudSideId(
        const EnsoDeviceId_t* deviceId,
        const char* cloudName)
{
    if (! (deviceId && cloudName) )
    {
        return eecNullPointerSupplied;
    }
    size_t cloudNameLength = strlen(cloudName);
    if (cloudNameLength == 0 || cloudNameLength > LSD_PROPERTY_NAME_MAX_LENGTH)
    {
        return eecParameterOutOfRange;
    }

    return LSD_RemoveProperty_Safe(deviceId, false, 0, cloudName, true);
}


/**
 * \name LSD_SetDeviceStatus
 *
 * \brief Set the thing status.
 *
 * \param theDevice                      The thing identifier
 *
 * \param deviceStatus                   The thing status
 *
 * \return                              A negative number denotes an error
 *                                      code
 */
EnsoErrorCode_e LSD_SetDeviceStatus(
        const EnsoDeviceId_t theDevice,
        const EnsoDeviceStatus_e deviceStatus)
{
    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal =  LSD_SetDeviceStatusDirectly(theDevice, deviceStatus);
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}


/**
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
EnsoErrorCode_e LSD_SetPropertyOutOfSync(
        const EnsoDeviceId_t* deviceId,
        const char* propName,
        const PropertyGroup_e propertyGroup)
{
    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal =  LSD_SetPropertyOutOfSyncDirectly(deviceId, propName, propertyGroup);
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}


/**
 * \name LSD_SetPropertyValueByAgentSideIdForObject
 *
 * \brief Used to set the value of a property.
 *
 * To set the property value using this function it must be a simple type: i.e.
 * evInt32, evUnsignedInt32, evFloat32 or evBoolean.
 *
 * \param source                        The id of the publisher (handler id)
 *
 * \param destination                   The ensoObject that shall "own" this
 *                                      property.
 *
 * \param agentSideId                   The property ID as seen on the agent
 *                                      side and provided by the ensoAgent
 *                                      component, typically the Device
 *                                      Application Layer and will often be
 *                                      based on physical characteristics such
 *                                      as ZigBee cluster and endpoint or
 *                                      Sprue identifiers.
 *
 * \param newValue                      The new value to set the property to.
 *
 * \return                              A negative number denotes an error
 *                                      code.
 *
 */
EnsoErrorCode_e LSD_SetPropertyValueByAgentSideIdForObject(
        const HandlerId_e source,
        EnsoObject_t* destination,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSideId,
        const EnsoPropertyValue_u newValue)
{
    /* Sanity checks */
    if (!destination)
    {
        return eecNullPointerSupplied;
    }

    if (propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }

    if (0 == agentSideId)
    {
        return eecPropertyNotFound;
    }

    // Set up a delta of a single property
    EnsoPropertyDelta_t delta[1];
    delta[0].agentSidePropertyID = agentSideId;
    delta[0].propertyValue = newValue;

    OSAL_LockMutex(&lsdMutex);
    // Set the properties to the new values
    uint16_t numPropertiesUpdated = LSD_SetPropertiesOfObjectDirectly(
            destination, propertyGroup, delta, 1);
    OSAL_UnLockMutex(&lsdMutex);

    EnsoErrorCode_e retVal = eecNoError;
    if (numPropertiesUpdated > 0)
    {
        retVal = LSD_NotifyDirectly(source, destination, propertyGroup, delta, 1);
    }

    return retVal;
}


/**
 * \name LSD_GetObjectPropertyValueByAgentSideId
 *
 * \brief   Retrieves the property value using its agent side identifier
 *
 * \param   ownerHandle         The ensoObject that owns the property in
 *                              question.
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \return                      eecNoError on success or the error code
 *                             (negative value) on failure.
 *
 */
EnsoErrorCode_e LSD_GetObjectPropertyValueByAgentSideId(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        EnsoPropertyValue_u* pRxValue)
{
    EnsoErrorCode_e retVal = eecNoError;

    /* Sanity checks */
    if (!owner || !pRxValue)
    {
        return eecNullPointerSupplied;
    }


    OSAL_LockMutex(&lsdMutex);
    EnsoProperty_t* property = LSD_FindPropertyByAgentSideIdDirectly(owner,  agentSidePropertyId);
    if (property )
    {
        switch (propertyGroup)
        {
            case REPORTED_GROUP:
                *pRxValue = property->reportedValue;
                break;

            case DESIRED_GROUP:
                *pRxValue = property->desiredValue;
                break;

            default:
                retVal = eecPropertyGroupNotSupported;
                break;
        }
    }
    else
    {
        retVal = eecPropertyNotFound;
    }
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}

/**
 * \name LSD_GetObjectPropertyBufferByAgentSideId
 *
 * \brief   Retrieves a large property value as identified by its Agent Side
 *          Identifier
 *
 * Large properties such as strings or buffers are not stored directly in the
 * property. Instead a handle is stored in the property which is used to
 * locate the buffer that actually stores the value.
 *
 * \param   owner               The ensoObject that owns the property in
 *                              question.
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \param   rxBufferSize        The size of the buffer that will receive the
 *                              property value.
 *
 * \param   pRxBuffer           A pointer to the buffer that will receive the
 *                              property value.
 *
 * \param   pBytesCopied        Receives the number of bytes copied - if the
 *                              buffer was empty or not allocated this will
 *                              be zero.
 *
 * \return                      A positive value is the number of bytes copied
 *                              into the buffer. eecNoError (0) isn't strictly
 *                              an error - it just means no bytes were copied,
 *                              probably as a result of passing a zero length
 *                              buffer.
 *
 *                              (negative value) on failure as enumerated by
 *                              EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_GetObjectPropertyBufferByAgentSideId(
        const EnsoObject_t* owner,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        const size_t rxBufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied)

{
    /*sanity checks */
    if ( ( NULL == owner ) || ( NULL == pRxBuffer ) || ( NULL == pBytesCopied ) )
    {
        return eecNullPointerSupplied;
    }

    if ( propertyGroup<0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }

    if ( 0 == agentSidePropertyId )
    {
        return eecPropertyNotFound;
    }

    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal = LSD_CopyFromPropertyBufferByAgentSideIdDirectly(
           owner,
           propertyGroup,
           agentSidePropertyId,
           rxBufferSize,
           pRxBuffer,
           pBytesCopied);
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}

/**
 * \name LSD_SubscribeToDevice
 *
 * \brief This function is used to subscribe to all properties in a
 *        property group for a device.
 *
 * \param   subjectDeviceId          The identifier of the thing to subscribe to
 *
 * \param   subjectPropertyGroup    The group to which the properties belongs to
 *
 * \param   subscribeId             The identifier of the subscriber
 *
 * \param   isSubscriberPrivate     Whether the subscriber is private
 *
 * \return                          EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_SubscribeToDevice(const EnsoDeviceId_t* subjectDeviceId,
        PropertyGroup_e subjectPropertyGroup,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate)
{
    /* sanity checks */
    if ( NULL == subjectDeviceId )
    {
        return eecNullPointerSupplied;
    }
    if ( subjectPropertyGroup<0 || subjectPropertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }

    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal =  LSD_SubscribeToDeviceDirectly(subjectDeviceId, subjectPropertyGroup, subscriberId, isSubscriberPrivate);
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}


/**
 * \name LSD_SubscribeToDevicePropertyByAgentSideId
 *
 * \brief This function is used to subscribe to a particular property in a
 * property group for a device.
 *
 * \param   subjectDeviceId          The identifier of the thing which owns the property
 *                                  to be subscribed to.
 *
 * \param   subjectAgentPropId      The agent side ID of the property to which
 *                                  to subscribe.
 *
 * \param   subjectPropertyGroup    The group to which the property belongs (so
 *                                  reported and desired states can have
 *                                  separate subscriber groups).
 *
 * \param   subscriberId            The identifier of the thing that is trying
 *                                  to subscribe to the updates.
 *
 * \param   isSubscriberPrivate     Whether the subscriber is private
 *
 * \return                          EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_SubscribeToDevicePropertyByAgentSideId(
        const EnsoDeviceId_t* subjectDeviceId,
        EnsoAgentSidePropertyId_t subjectAgentPropId,
        PropertyGroup_e subjectPropertyGroup,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate)
{
    /*sanity checks */
    if ( NULL == subjectDeviceId )
    {
        return eecNullPointerSupplied;
    }
    if ( subjectPropertyGroup<0 || subjectPropertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }
    if ( 0 == subjectAgentPropId )
    {
        return eecPropertyNotFound;
    }
    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal =  LSD_SubscribeToDevicePropertyByAgentSideIdDirectly(subjectDeviceId, subjectAgentPropId, subjectPropertyGroup, subscriberId, isSubscriberPrivate);
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}


/*****************************************************************************/
/*                                                                           */
/* Thing based functions                                                     */
/*                                                                           */
/*****************************************************************************/

/**
 * \name LSD_FindEnsoObjectByDeviceId
 *
 * \brief Find an ensoObject by its physical identifier.
 *
 * This function is used to find an ensoObject by its physical identifier.
 *
 * \param  deviceId          The thing ID of the device to look for,
 *                          probably based on a MAC address or ZigBee
 *                          device ID.
 *
 * \return                  Success - pointer to ensoObject
 *                          Failure NULL
 *
 */
EnsoObject_t* LSD_FindEnsoObjectByDeviceId(const EnsoDeviceId_t* deviceId)
{
    EnsoObject_t* theObject = NULL;

    //assert(deviceId);

    if (deviceId)
    {
        OSAL_LockMutex(&lsdMutex);

        theObject = LSD_FindEnsoObjectByDeviceIdDirectly( deviceId );

        OSAL_UnLockMutex(&lsdMutex);
    }

    return theObject;
}


/**
 * \brief   Get the list of devices that are managed by the handler specified as
 * an argument
 *
 * \param   handlerId       The identifier of the handler managing the devices
 *
 * \param[out] buffer       Buffer where the list of device identifiers is written

 * \param   bufferElems     Maximum number of elements of the buffer array
 *
 * \param[out] numDevices   Number of device identifiers that have been written.
 *                          If the function returns successfully then this is also
 *                          the number of devices that are managed by the handler
 *
 * \return                  EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_GetDevicesId(
        const HandlerId_e handlerId,
        EnsoDeviceId_t* buffer,
        const uint16_t  bufferElems,
        uint16_t* numDevices)
{
    // Sanity check
    if (!buffer || !numDevices)
    {
        return eecNullPointerSupplied;
    }

    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal = LSD_GetDevicesIdDirectly(handlerId, buffer, bufferElems, numDevices);
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}

/**
 * \name LSD_GetPropertyCloudNameFromAgentSideId
 *
 * \brief Retrieves a property name by its agent side identifer.
 *
 * This function finds the property name using the agent side property identifier.
 *
 * \param   deviceID            The device that owns the property in
 *                              question.
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \param   rxBufferSize        The size of the buffer that will receive the cloud name
 *
 * \param   cloudName           Buffer to copy name into
 *
 * \return                      EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_GetPropertyCloudNameFromAgentSideId(const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSidePropertyId, const size_t bufferSize,
        char* cloudName)
{
    bool result = eecPropertyNotFound;

    if ((NULL == deviceId) || (NULL == cloudName))
    {
        return eecNullPointerSupplied;
    }

    if (LSD_PROPERTY_NAME_BUFFER_SIZE > bufferSize)
    {
        result = eecBufferTooSmall;
    }
    else
    {
        EnsoProperty_t* property  = LSD_GetPropertyByAgentSideId(
                deviceId, agentSidePropertyId);

        if (property)
        {
            strncpy(cloudName, property->cloudName, LSD_PROPERTY_NAME_BUFFER_SIZE);
            result = eecNoError;
        }
    }

    return result;
}


/**
 * \name LSD_GetPropertyByCloudName
 *
 * \brief   Retrieves a pointer to the property structure.
 *
 * This function retrieves a pointer to the property structure using its
 * ensoCloud shadow name to find it.
 *
 * \param   ownerHandle         The ensoObject that owns the property in
 *                              question.
 *
 * \param   cloudName           The name of the property as it is used by the
 *                              ensoCloud side of the shadow in null terminated
 *                              string form.
 *
 * \return                      Pointer to the property or NULL if not
 *                              successful.
 *
 */
EnsoProperty_t* LSD_GetPropertyByCloudName(
        const EnsoDeviceId_t* deviceId,
        const char* cloudName)
{
    EnsoProperty_t* theProperty = NULL;

    if ( ( NULL == deviceId) || ( NULL == cloudName ) )
    {
        return NULL;
    }

    /*sanity checks */

    OSAL_LockMutex(&lsdMutex);

    EnsoObject_t* thingObject = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if ( thingObject )
    {
        theProperty = LSD_FindPropertyByCloudSideIdDirectly(thingObject, cloudName );
    }

    OSAL_UnLockMutex(&lsdMutex);

    return theProperty;
}

/**
 * \name LSD_GetPropertyValueByCloudName
 *
 * \brief   Retrieves the value of the property.
 *
 * This function retrieves the value of a property (in the case of large
 * properties the handle to that property).
 *
 * \param   ownerHandle         The ensoObject that owns the property in
 *                              question.
 *
 * \param   cloudName           The name of the property as it is used by the
 *                              ensoCloud side of the shadow in null terminated
 *                              string form.
 *
 * \param   pRxValue            A pointer to the variable that shall receive
 *                              the value of the property.
 *
 * \return                      eecNoError on success or the error code
 *                             (negative value) on failure.
 *
 */
EnsoErrorCode_e LSD_GetPropertyValueByCloudName(
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char* cloudName,
        EnsoPropertyValue_u* pRxValue)
{
    /* Sanity checks */
    if ( ( NULL == deviceId) || ( NULL == pRxValue ) )
    {
        return eecNullPointerSupplied;
    }
    if ( propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX )
    {
        return eecPropertyGroupNotSupported;
    }

    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;
    EnsoObject_t* thingObject = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if ( thingObject )
    {
        retVal = LSD_GetPropertyValueByCloudSideIdDirectly(
                    thingObject, propertyGroup, cloudName, pRxValue);
    }
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}

/**
 * \name LSD_GetPropertyByAgentSideId
 *
 * \brief   Retrieves a property from a thing
 *
 * This function retrieves a pointer to the property structure using its
 * handle to identify it.
 *
 * \param   deviceId             The thing that owns the property in
 *                              question.
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \return                      SUCCESS : Pointer to the property
 *                              FAILURE : NULL
 *
 */
EnsoProperty_t* LSD_GetPropertyByAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSidePropertyId)
{

    EnsoProperty_t* theProperty = NULL;

    if ( deviceId )
    {
        OSAL_LockMutex(&lsdMutex);
        EnsoObject_t* thingObject = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
        if ( thingObject )
        {
            theProperty = LSD_FindPropertyByAgentSideIdDirectly( thingObject,  agentSidePropertyId);
        }
        OSAL_UnLockMutex(&lsdMutex);
    }

    return theProperty;
}

/**
 * \name LSD_IsPropertyBufferedByAgendSideId
 *
 * \brief   Retrieves the buffered status for a property
 *
 * \param   deviceId            The thing that owns the property in
 *                              question.
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \return  true if property is buffered, false otherwise
 *
 */
bool LSD_IsPropertyBufferedByAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSidePropertyId)
{

    EnsoProperty_t* theProperty = NULL;

    if ( deviceId )
    {
        OSAL_LockMutex(&lsdMutex);
        EnsoObject_t* thingObject = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
        if ( thingObject )
        {
            theProperty = LSD_FindPropertyByAgentSideIdDirectly( thingObject, agentSidePropertyId);
        }
        OSAL_UnLockMutex(&lsdMutex);
    }

    return theProperty->type.buffered;
}

/**
 * \name LSD_GetPropertyValueByAgentSideId
 *
 * \brief   Retrieves the property value using its agent side identifier
 *
 * This function retrieves a pointer to the property structure using the DAL
 * property identifier.
 *
 * \param   ownerHandle         The ensoObject that owns the property in
 *                              question.
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \return                      eecNoError on success or the error code
 *                             (negative value) on failure.
 *
 */
EnsoErrorCode_e LSD_GetPropertyValueByAgentSideId(
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        EnsoPropertyValue_u* pRxValue)
{
    EnsoErrorCode_e retVal = eecNullPointerSupplied;

    /*sanity checks */

    if ( deviceId && pRxValue )
    {
        OSAL_LockMutex(&lsdMutex);
        EnsoObject_t* thingObject = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
        if ( thingObject )
        {
            EnsoProperty_t* theProperty = LSD_FindPropertyByAgentSideIdDirectly( thingObject,  agentSidePropertyId);
            if ( NULL != theProperty )
            {
                switch ( propertyGroup )
                {
                case DESIRED_GROUP:
                    *pRxValue = theProperty->desiredValue;
                    retVal = eecNoError;
                    break;

                case REPORTED_GROUP:
                    *pRxValue = theProperty->reportedValue;
                    retVal = eecNoError;
                    break;

                default:
                    retVal = eecPropertyGroupNotSupported;
                    break;
                }
            }
            else
            {
                retVal = eecPropertyNotFound;
            }
        }
        else
        {
        	LOG_Error("Failed to find thing object in local shadow");
        	retVal = eecEnsoObjectNotFound;
        }
        OSAL_UnLockMutex(&lsdMutex);
    }

    return retVal;
}

/**
 * \name LSD_GetPropertyBufferByCloudName
 *
 * \brief   Retrieves a large property value as identified by its Device
 *          Abstraction Layer (DAL) identifier.
 *
 * Large properties such as strings or buffers are not stored directly in the
 * property. Instead a handle is stored in the property which is used to
 * locate the buffer that actually stores the value.
 *
 * \param   ownerHandle         The ensoObject that owns the property in
 *                              question.
 *
 * \param   cloudName           The name of the property as it is used by the
 *                              ensoCloud side of the shadow in null terminated
 *                              string form.
 *
 * \param   rxBufferSize        The size of the buffer that will receive the
 *                              property value.
 *
 * \param   pRxBuffer           A pointer to the buffer that will receive the
 *                              property value.
 *
 * \param   pBytesCopied        Receives the number of bytes copied - if the
 *                              buffer was empty or not allocated this will
 *                              be zero.
 *
 * \return                      A positive value is the number of bytes copied
 *                              into the buffer. eecNoError (0) isn't strictly
 *                              an error - it just means no bytes were copied,
 *                              probably as a result of passing a zero length
 *                              buffer.
 *
 *                              (negative value) on failure as enumerated by
 *                              EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_GetPropertyBufferByCloudName(
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char* cloudName,
        const size_t rxBufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied )

{
    /*sanity checks */
    if ( ( NULL == deviceId) || ( NULL == cloudName ) ||
         ( NULL == pRxBuffer ) || ( NULL == pBytesCopied ) )
    {
        return eecNullPointerSupplied;
    }
    if ( propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX )
    {
        return eecPropertyGroupNotSupported;
    }
    if ( 0 == rxBufferSize )
    {
        return eecBufferTooSmall;
    }

    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;
    EnsoObject_t* object = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if ( NULL != object )
    {
        retVal = LSD_CopyFromPropertyBufferByCloudSideIdDirectly(
                    object, propertyGroup, cloudName,
                    rxBufferSize, pRxBuffer, pBytesCopied);
    }
    OSAL_UnLockMutex(&lsdMutex);

    return retVal;
}

/**
 * \name LSD_GetPropertyBufferByAgentSideId
 *
 * \brief   Retrieves a large property value as identified by its Agent Side
 *          Identifier
 *
 * Large properties such as strings or buffers are not stored directly in the
 * property. Instead a handle is stored in the property which is used to
 * locate the buffer that actually stores the value.
 *
 * \param   ownerHandle         The ensoObject that owns the property in
 *                              question.
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \param   rxBufferSize        The size of the buffer that will receive the
 *                              property value.
 *
 * \param   pRxBuffer           A pointer to the buffer that will receive the
 *                              property value.
 *
 * \param   pBytesCopied        Receives the number of bytes copied - if the
 *                              buffer was empty or not allocated this will
 *                              be zero.
 *
 * \return                      A positive value is the number of bytes copied
 *                              into the buffer. eecNoError (0) isn't strictly
 *                              an error - it just means no bytes were copied,
 *                              probably as a result of passing a zero length
 *                              buffer.
 *
 *                              (negative value) on failure as enumerated by
 *                              EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_GetPropertyBufferByAgentSideId(const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        const size_t rxBufferSize,
        void* pRxBuffer,
        size_t* pBytesCopied)

{
    /*sanity checks */
    if ( ( NULL == deviceId) || ( NULL == pRxBuffer ) || ( NULL == pBytesCopied ) )
    {
        return eecNullPointerSupplied;
    }
    if ( propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX )
    {
        return eecPropertyGroupNotSupported;
    }
    if ( 0 == rxBufferSize )
    {
        return eecBufferTooSmall;
    }
    if ( 0 == agentSidePropertyId )
    {
        return eecPropertyNotFound;
    }

    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;
    EnsoObject_t* object = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if ( NULL != object )
    {
        retVal = LSD_CopyFromPropertyBufferByAgentSideIdDirectly(
                    object, propertyGroup, agentSidePropertyId,
                    rxBufferSize, pRxBuffer, pBytesCopied);
    }
    OSAL_UnLockMutex(&lsdMutex);
    return retVal;
}


/**
 * \name LSD_SetPropertyValueByCloudName
 *
 * \brief   Sets the value of the property.
 *
 * This function sets the value of a property using its handle to identify it.
 *
 * \param   source              The id of the publisher (handler id)
 *
 * \param   ownerHandle         The ensoObject that owns the property in
 *                              question.
 *
 * \param   cloudName           The name of the property as it is used by the
 *                              ensoCloud side of the shadow in null terminated
 *                              string form.
 *
 * \param   newValue            A new value to assign.
 *
 * \return                      eecNoError on success or the error code
 *                             (negative value) on failure.
 *
 */
EnsoErrorCode_e LSD_SetPropertyValueByCloudName(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char* cloudName,
        const EnsoPropertyValue_u newValue)
{
    /* Sanity checks */
    if (!deviceId || !cloudName)
    {
        return eecNullPointerSupplied;
    }

    if (propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }

    // Set up a delta of a single property
    EnsoPropertyDelta_t delta[1];
    delta[0].propertyValue = newValue;

    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;
    uint16_t numPropertiesUpdated = 0;

    OSAL_LockMutex(&lsdMutex);
    // Set the properties to the new values
    EnsoObject_t* destination = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if (destination)
    {
        retVal = eecPropertyNotFound;
        EnsoProperty_t* property = LSD_FindPropertyByCloudSideIdDirectly(destination, cloudName);
        if (property)
        {
            retVal = eecNoChange;
            delta[0].agentSidePropertyID = property->agentSidePropertyID;
            numPropertiesUpdated = LSD_SetPropertiesOfObjectDirectly(
                    destination, propertyGroup, delta, 1);
        }
    }
    OSAL_UnLockMutex(&lsdMutex);

    if (numPropertiesUpdated > 0)
    {
        retVal = LSD_NotifyDirectly(source, destination, propertyGroup, delta, 1);
    }

    return retVal;
}



/**
 * \name LSD_SetPropertyValueByCloudNameWithoutNotification
 *
 * \brief   Sets the value of the property and does not notify, but returns
 *          a delta so that we can collate deltas together. All desired
 *          properties from the cloud should be process with this function. It
 *          guarantees that the local shadow contains all the desired changes
 *          that have been received, before any deltas are sent to device handlers.
 *
 * \param   source              The id of the publisher (handler id)
 *
 * \param   deviceID            The ensoObject that owns the property in
 *                              question.
 *
 * \param   propertyGroup       Property group these changes are for
 *
 * \param   cloudName           The name of the property as it is used by the
 *                              ensoCloud side of the shadow in null terminated
 *                              string form.
 *
 * \param   newValue            A new value to assign.
 *
 * \param   delta               delta buffer to fill
 *
 * \param   deltaCounter        current index for delta buffer
 *
 * \return                      eecNoError on success or the error code
 *                              (negative value) on failure.
 *
 */
EnsoErrorCode_e LSD_SetPropertyValueByCloudNameWithoutNotification(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char* cloudName,
        const EnsoPropertyValue_u newValue,
        EnsoPropertyDelta_t* delta,
        int* deltaCounter)
{
    /* Sanity checks */
    if (!deviceId || !cloudName)
    {
        return eecNullPointerSupplied;
    }

    if (propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }

    // Set up a delta of a single property
    delta[*deltaCounter].propertyValue = newValue;

    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;
    uint16_t numPropertiesUpdated = 0;

    OSAL_LockMutex(&lsdMutex);
    // Set the properties to the new values
    EnsoObject_t* destination = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if (destination)
    {
        retVal = eecPropertyNotFound;
        EnsoProperty_t* property = LSD_FindPropertyByCloudSideIdDirectly(destination, cloudName);
        if (property)
        {
            retVal = eecNoChange;
            delta[*deltaCounter].agentSidePropertyID = property->agentSidePropertyID;
            numPropertiesUpdated = LSD_SetPropertiesOfObjectDirectly(
                    destination, propertyGroup, &delta[*deltaCounter], 1);
        }
    }
    OSAL_UnLockMutex(&lsdMutex);

    if (numPropertiesUpdated > 0)
    {
        (*deltaCounter)++;
        retVal = eecNoError;
    }

    return retVal;
}




/**
 * \name LSD_SetPropertyValueByAgentSideId
 *
 * \brief   Sets the value of the property found using its agent side id
 *
 * This function sets the value of a property using the agent side identifier
 * to find it.
 *
 * \param   source              The id of the publisher (handler id)
 *
 * \param   ownerHandle         The ensoObject that owns the property in
 *                              question.
 *
 * \param   agentSideId         The property ID as supplied by the agent side
 *
 * \param   newValue            A new value to assign.
 *
 * \return                      eecNoError on success or the error code
 *                             (negative value) on failure.
 *
 */
EnsoErrorCode_e LSD_SetPropertyValueByAgentSideId(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSideId,
        const EnsoPropertyValue_u newValue)
{
    /* Sanity checks */
    if (!deviceId)
    {
        return eecNullPointerSupplied;
    }

    if (propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }

    if (0 == agentSideId)
    {
        return eecPropertyNotFound;
    }

    // Set up a delta of a single property
    EnsoPropertyDelta_t delta[1];
    delta[0].agentSidePropertyID = agentSideId;
    delta[0].propertyValue = newValue;

    EnsoErrorCode_e retVal = eecNoError;
    uint16_t numPropertiesUpdated = 0;

    OSAL_LockMutex(&lsdMutex);
    // Set the properties to the new values
    EnsoObject_t* destination = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if (destination)
    {
        numPropertiesUpdated = LSD_SetPropertiesOfObjectDirectly(
                destination, propertyGroup, delta, 1);
        retVal = eecNoError;
    }
    else
    {
        retVal = eecEnsoObjectNotFound;
    }
    OSAL_UnLockMutex(&lsdMutex);

    if (eecNoError == retVal && numPropertiesUpdated > 0)
    {
        retVal = LSD_NotifyDirectly(source, destination, propertyGroup, delta, 1);
    }

    return retVal;
}

/**
 * \name LSD_SetPropertyBufferByCloudName
 *
 * \brief   Sets the value of the property.
 *
 * This function sets the value of a property using its handle to identify it.
 *
 * \param   source              The id of the publisher (handler id)
 *
 * \param   deviceId            The ensoObject that owns the property in
 *                              question.
 *
 * \param   propertyGroup       The group of the property in the local shadow
 *                              (currently desired and reported states but could
 *                              be extended).
 *
 * \param   cloudName           The name of the property as it is used by the
 *                              ensoCloud side of the shadow in null terminated
 *                              string form.
 *
 * \param   bufferSize          The size of the buffer that contains the new
 *                              property value.
 *
 * \param   pNewBuffer          A pointer to the buffer that will contains the
 *                              new property value.
 *
 * \param   pBytesCopied        Receives the number of bytes copied - if the
 *                              buffer was empty or not allocated this will
 *                              be zero.
 *
 * \return                      eecNoError on success or the error code
 *                             (negative value) on failure.
 *
 */
EnsoErrorCode_e LSD_SetPropertyBufferByCloudName(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const char *cloudName,
        const size_t bufferSize,
        const void* pNewBuffer,
        size_t* pBytesCopied)
{
    /* Sanity checks */
    if (!deviceId || !cloudName || !pNewBuffer || !pBytesCopied)
    {
        return eecNullPointerSupplied;
    }
    if (propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }
    if (0 == bufferSize)
    {
        return eecBufferTooSmall;
    }

    OSAL_LockMutex(&lsdMutex);
    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;
    EnsoObject_t* object = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    EnsoProperty_t* property = NULL;
    if (object)
    {
        retVal = eecPropertyNotFound;
        property = LSD_FindPropertyByCloudSideIdDirectly(object, cloudName);
        if (property)
        {
            retVal = LSD_CopyBufferToPropertyDirectly(property,
                    propertyGroup, bufferSize, pNewBuffer);
        }
    }

    *pBytesCopied = (eecNoError == retVal) ? bufferSize : 0;

    OSAL_UnLockMutex(&lsdMutex);

    if (eecNoError == retVal)
    {
        EnsoPropertyDelta_t delta[1];

        // Set up a delta of a single property
        delta[0].agentSidePropertyID = property->agentSidePropertyID; // cppcheck-suppress nullPointer
        EnsoPropertyValue_u* pValue = LSD_GetPropertyValuePtrFromGroupDirectly(property, propertyGroup);
        assert(pValue); // Should not be NULL here
        delta[0].propertyValue = *pValue;
        retVal = LSD_NotifyDirectly(source, object, propertyGroup, delta, 1);
    }

    return retVal;
}

/**
 * \name LSD_SetPropertyBufferByAgentSideId
 *
 * \brief   Sets the property buffer using the agent side property
 * identifier to find it.
 *
 * \param   source              The id of the publisher (handler id)
 *
 * \param   deviceId            The ensoObject that owns the property in
 *                              question.
 *
 * \param   propertyGroup       The group of the property in the local shadow
 *                              (currently desired and reported states but could
 *                              be extended).
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \param   newValue            A new value to assign.
 *
 * \param   pBytesCopied        Receives the number of bytes copied - if the
 *                              buffer was empty or not allocated this will
 *                              be zero.
 *
 * \return                      eecNoError on success or the error code
 *                              (negative value) on failure.
 *
 */
EnsoErrorCode_e LSD_SetPropertyBufferByAgentSideId(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoAgentSidePropertyId_t agentSidePropertyId,
        const size_t bufferSize,
        const void* pNewBuffer,
        size_t* pBytesCopied)
{
    /* Sanity checks */
    if (!deviceId || !pNewBuffer || !pBytesCopied)
    {
        return eecNullPointerSupplied;
    }
    if (propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }
    if (0 == bufferSize)
    {
        return eecBufferTooSmall;
    }
    if (0 == agentSidePropertyId)
    {
        return eecPropertyNotFound;
    }

    OSAL_LockMutex(&lsdMutex);

    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;
    EnsoObject_t* owner = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    EnsoProperty_t* property = NULL;
    if (owner)
    {
        retVal = eecPropertyNotFound;
        property = LSD_FindPropertyByAgentSideIdDirectly(owner, agentSidePropertyId);
        if (property)
        {
            retVal = LSD_CopyBufferToPropertyDirectly(property,
                    propertyGroup, bufferSize, pNewBuffer);
        }
    }

    *pBytesCopied = (eecNoError == retVal) ? bufferSize : 0;

    OSAL_UnLockMutex(&lsdMutex);

    if (eecNoError == retVal)
    {
        EnsoPropertyDelta_t delta[1];

        // Set up a delta of a single property
        delta[0].agentSidePropertyID = agentSidePropertyId;
        EnsoPropertyValue_u* pValue = LSD_GetPropertyValuePtrFromGroupDirectly(property, propertyGroup);
        assert(pValue); // Should not be NULL here
        delta[0].propertyValue = *pValue;
        retVal = LSD_NotifyDirectly(source, owner, propertyGroup, delta, 1);
    }

    return retVal;
}


/*****************************************************************************/
/*                                                                           */
/* Update cloud                                                              */
/*                                                                           */
/*****************************************************************************/


/*
 * \brief Set the properties of the thing.
 *
 * This function looks up the object corresponding to the thing in the local
 * shadow, sets the properties and calls the notification functions on all
 * the subscriber objects.
 *
 * \param   source          The id of the publisher (handler id)
 *
 * \param   deviceId        The object which properties are being modified
 *
 * \param   propertyGroup   The group of the property in the local shadow
 *                          (currently desired and reported states but could
 *                          be extended).
 *
 * \param   propertyDelta   The block of property agent side identifiers and
 *                          their new values.
 *
 * \param   numProperties   The number of properties in propertyDelta.
 *
 * \return                  ensoError
 */
EnsoErrorCode_e LSD_SetPropertiesOfDevice(
        const HandlerId_e source,
        const EnsoDeviceId_t* deviceId,
        const PropertyGroup_e propertyGroup,
        const EnsoPropertyDelta_t* propertyDelta,
        const uint16_t numProperties)
{
    /* Sanity checks */
    if (!deviceId || !propertyDelta)
    {
        return eecNullPointerSupplied;
    }

    if (propertyGroup < 0 || propertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecPropertyGroupNotSupported;
    }

    if (numProperties > ECOM_MAX_DELTAS)
    {
        return eecBufferTooBig;
    }

    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;
    uint16_t numPropertiesUpdated = 0;

    OSAL_LockMutex(&lsdMutex);
    // Set the properties to the new values
    EnsoObject_t* destination = LSD_FindEnsoObjectByDeviceIdDirectly(deviceId);
    if (destination)
    {
        numPropertiesUpdated = LSD_SetPropertiesOfObjectDirectly(
                destination, propertyGroup, propertyDelta, numProperties);
        retVal = eecNoError; // Object is found, but maybe nothing to update
    }
    OSAL_UnLockMutex(&lsdMutex);

    if (numPropertiesUpdated > 0)
    {
        retVal = LSD_NotifyDirectly(source, destination, propertyGroup, propertyDelta, numProperties);
    }

    return retVal;
}



/**
 * \name prv_NotifyStorageOfPersistentProperty
 *
 * \brief Make the storage handler aware of a new persistent property.
 *
 * \param   owner                       The ensoObject that owns this property
 *
 * \param   agentSideId                 The property ID as seen on the agent
 *                                      side
 *
 * \return                              A negative number denotes an error
 *                                      code
 */
static EnsoErrorCode_e prv_NotifyStorageOfPersistentProperty(
        EnsoObject_t* owner,
        const EnsoAgentSidePropertyId_t agentSideId)
{
    // Notify storage handler directly so that both desired and reported are recorded in storage.
    EnsoPropertyDelta_t delta;
    delta.agentSidePropertyID = agentSideId;

    EnsoPropertyValue_u value;

    EnsoErrorCode_e retVal = LSD_GetObjectPropertyValueByAgentSideId(owner, DESIRED_GROUP, agentSideId, &value);
    if (retVal < 0)
    {
        LOG_Error("Failed to get property desired value for %x", agentSideId);
    }
    else
    {
        delta.propertyValue = value;

        retVal = ECOM_SendUpdateToSubscriber(STORAGE_HANDLER, owner->deviceId, DESIRED_GROUP, 1, &delta);
        if (retVal < 0)
        {
            LOG_Error("Failed to send desired for %x storage handler", agentSideId);
        }
    }

    retVal = LSD_GetObjectPropertyValueByAgentSideId(owner, REPORTED_GROUP, agentSideId, &value);
    if (retVal < 0)
    {
        LOG_Error("Failed to get property reported value for %x", agentSideId);
    }
    else
    {
        delta.propertyValue = value;

        retVal = ECOM_SendUpdateToSubscriber(STORAGE_HANDLER, owner->deviceId, REPORTED_GROUP, 1, &delta);
        if (retVal < 0)
        {
            LOG_Error("Failed to send reported for %x storage handler", agentSideId);
        }
    }

    // Subscribe storage handler to the property
    retVal = LSD_SubscribeToDevicePropertyByAgentSideId(&owner->deviceId,
            agentSideId, DESIRED_GROUP, STORAGE_HANDLER, true);
    if (retVal < 0)
    {
        LOG_Error("Failed to subscribe storage handler to %x desired", agentSideId);
    }

    retVal = LSD_SubscribeToDevicePropertyByAgentSideId(&owner->deviceId,
            agentSideId, REPORTED_GROUP, STORAGE_HANDLER, true);

    if (retVal < 0)
    {
        LOG_Error("Failed to subscribe storage handler to %x reported", agentSideId);
    }

    return retVal;
}



/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * \brief   Consumer thread - receives messages on mq
 *
 * \param   mq Message queue on which to listen.
 */
static void _LSD_MessageQueueListener(MessageQueue_t mq)
{
    // For ever loop
    for (; ; )
    {
        char buffer[ECOM_MAX_MESSAGE_SIZE];
        MessagePriority_e priority;

        int size = OSAL_ReceiveMessage(mq, buffer, sizeof(buffer), &priority);
        if (size < 1)
        {
            LOG_Error("ReceiveMessage error %d", size);
            continue;
        }

        // Read the message id
        uint8_t messageId = buffer[0];
        switch (messageId)
        {
            case ECOM_DUMP_LOCAL_SHADOW:
            {
                LOG_Info("***** Dump LOCAL SHADOW *****");

                // Dump local shadow in the new store.
                // It is possible that deltas from devices or the cloud get interleaved with
                // those being dumped from the local shadow. This is not a problem as the 
                // latest value is always read from the shadow by storage handler

                uint16_t sentDeltas;
                EnsoErrorCode_e retVal = LSD_SendPropertiesByFilter(
                        STORAGE_HANDLER,
                        REPORTED_GROUP,
                        PROPERTY_FILTER_PERSISTENT,
                        &sentDeltas);
                if (eecNoError != retVal)
                {
                    LOG_Error("LSD_SendPropertiesByFilter for reported failed %s", LSD_EnsoErrorCode_eToString(retVal));
                }

                retVal = LSD_SendPropertiesByFilter(
                        STORAGE_HANDLER,
                        DESIRED_GROUP,
                        PROPERTY_FILTER_PERSISTENT,
                        &sentDeltas);
                if (eecNoError != retVal)
                {
                    LOG_Error("LSD_SendPropertiesByFilter for desired failed %s", LSD_EnsoErrorCode_eToString(retVal));
                }
            }
            break;


            default:
                LOG_Error("Unknown message %d", messageId);
                break;
        }
    }

    LOG_Warning("MessageQueueListener exit");
}

