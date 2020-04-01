/*!****************************************************************************
 *
 * \file LSD_EnsoObjectStore.c
 *
 * \author Carl Pickering
 *
 * \brief The property store for the ensoObjects
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <limits.h>
//#include <inttypes.h>
#include <stdint.h>
#include "LOG_Api.h"
#include "ECOM_Api.h"
#include "CLD_CommsInterface.h"

#include "LSD_EnsoObjectStore.h"
#include "LSD_PropertyStore.h"
#include "LSD_Subscribe.h"
#include "ECOM_Messages.h"
#include "SYS_Gateway.h"
#include "APP_Types.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/


/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

/**
 * \name prv_ObjectStore
 *
 * \brief Simple array structure to store the object pointers
 */
static EnsoObject_t prv_ObjectStore[LSD_MAX_THING + 1];


/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * \name prv_SetDeviceStatus
 *
 * \brief Set the device status.
 *
 * \param theObject        Pointer to the enso object for the device
 *
 * \param deviceStatus     The new device status
 */
static void prv_SetDeviceStatus(
        EnsoObject_t* theObject,
        const EnsoDeviceStatus_e deviceStatus)
{
    assert(theObject);

    EnsoPropertyValue_u newValue;
    newValue.uint32Value = deviceStatus;
    EnsoErrorCode_e retVal = LSD_SetPropertyValueByAgentSideIdDirectly(
            theObject,
            REPORTED_GROUP,
            PROP_DEVICE_STATUS_ID,
            newValue);

    if ((eecNoError != retVal) && (eecNoChange != retVal))
    {
        LOG_Error("LSD_SetPropertyValueByAgentSideIdDirectly failed %s", LSD_EnsoErrorCode_eToString(retVal));
    }

    if (THING_REJECTED == deviceStatus)
    {
#if 0
        LOG_Error("Thing (%016"__PRIx64"_%04x_%02x) is rejected, removing subscribers!",
                  theObject->deviceId.deviceAddress,
                  theObject->deviceId.technology,
                  theObject->deviceId.childDeviceId);
#endif
        /* Remove any subscriber for that object */
        memset(theObject->subscriptionBitmap, 0, sizeof(theObject->subscriptionBitmap));

    }
}

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * \name LSD_DeviceIdCompare
 *
 * \brief Compare two thing identifiers analogous to strcmp
 *
 * \param leftThing     The thing on the left to compare
 *
 * \param rightThing    The thing on the right to compare
 *
 * \return              positive leftThing >  rightThing
 *                      zero     leftThing == rightThing
 *                      negative leftThing <  rightThing
 *
 */
int LSD_DeviceIdCompare(const EnsoDeviceId_t* leftThing, const EnsoDeviceId_t* rightThing)
{
    assert(leftThing);
    assert(rightThing);

    int comparison = 0;

    if (leftThing->deviceAddress > rightThing->deviceAddress)
    {
        comparison = 1;
    }
    else if (leftThing->deviceAddress < rightThing->deviceAddress)
    {
        comparison = -1;
    }
    else
    {
        if (leftThing->technology > rightThing->technology)
        {
            comparison = 1;
        }
        else if (leftThing->technology < rightThing->technology)
        {
            comparison = -1;
        }
        else
        {
            if (leftThing->childDeviceId > rightThing->childDeviceId)
            {
                comparison = 1;
            }
            else if (leftThing->childDeviceId < rightThing->childDeviceId)
            {
                comparison = -1;
            }
            else
            {
                comparison = 0;
            }
        }
    }

    return comparison;
}

/*
 * \brief   This function checks to see if the ensoThing ID is valid.
 *
 * \param   theDevice    The thing to look at.
 *
 * \return              True if valid.
 */
bool LSD_IsEnsoDeviceIdValid(const EnsoDeviceId_t theDevice)
{
    bool valid = true;

    if (0 == theDevice.technology &&
        0 == theDevice.childDeviceId &&
        0 == theDevice.deviceAddress)
    {
        valid = false;
    }

    return valid;
}

/*
 * \brief This function copies a thing ID.
 *
 * \param   dest        The destination space for the thingID.
 *
 * \param   source      The source deviceId to be copied.
 *
 * \return              EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_CopyDeviceId(EnsoDeviceId_t* dest, const EnsoDeviceId_t* source)
{
    if (!dest || !source)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    if (!LSD_IsEnsoDeviceIdValid(*source))
    {
        return eecEnsoObjectNotFound;
    }

    dest->technology = source->technology;
    dest->childDeviceId = source->childDeviceId;
    dest->deviceAddress = source->deviceAddress;
    dest->isChild = source->isChild;
    return eecNoError;
}


/**
 * \name LSD_EnsoObjectStoreInit
 *
 * \brief Used to set up the enso object store.
 *
 * \return ensoError        Most likely to be out of space...
 *
 */
EnsoErrorCode_e LSD_EnsoObjectStoreInit(void)
{
    EnsoErrorCode_e retVal = eecNoError;

    memset(prv_ObjectStore, 0, sizeof(prv_ObjectStore));

    return retVal;
}


/**
 * \name LSD_CreateEnsoObjectDirectly
 *
 * \brief Initialise a new ensoObject directly in the shadow
 *
 * NOT THREAD SAFE
 *
 * This function is used to create an ensoObject.
 *
 * \param  newId            The unique thing identifier
 *
 * \return EnsoObject_t*    Pointer to the new ensoObject
 *
 */
EnsoObject_t* LSD_CreateEnsoObjectDirectly(
    const EnsoDeviceId_t newId)
{
    if (!LSD_IsEnsoDeviceIdValid(newId))
    {
        return NULL;
    }

    /* Run through store to see if object already exists */
    EnsoObject_t* newObject = LSD_FindEnsoObjectByDeviceIdDirectly(&newId);
    if (newObject)
    {
        LOG_Warning("Object already exists");
        return newObject;
    }

    /* Now find a slot for the object */
    int i = 0;
    bool stored = false;

    while ((i <= LSD_MAX_THING) && (!stored))
    {
        if (!LSD_IsEnsoDeviceIdValid(prv_ObjectStore[i].deviceId))
        {
            newObject = &prv_ObjectStore[i];
            stored = true;
            break;
        }
        i++;
    }

    if (!stored)
    {
        LOG_Error("Unable to create EnsoObject, out of memory");
    }
    else
    {
        // Initialise the new object.
        newObject->deviceId = newId;
        newObject->propertyListStart = -1;

        // Not announced yet to cloud
        newObject->announceAccepted = false;

        newObject->announceInProgress = false;

        // No properties yet, can't be out of sync
        newObject->reportedOutOfSync = false;
        newObject->desiredOutOfSync = false;
    }

    return newObject;
}


/**
 * \brief Remove a device from the store.
 *
 * \param deviceId      Id of the device to destroy
 *
 * \return              EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_DestroyEnsoDeviceDirectly(const EnsoDeviceId_t deviceId)
{
    EnsoErrorCode_e retVal = eecEnsoObjectNotFound;

    EnsoObject_t* object = LSD_FindEnsoObjectByDeviceIdDirectly(&deviceId);
    if (object)
    {
        // Invalidate the object (an invalid device ID is used to indicate the
        // object isn't valid)
        memset(object, 0, sizeof(EnsoObject_t));
        retVal = eecNoError;
    }

    return retVal;
}


/**
 * \name LSD_RegisterEnsoObject
 *
 * \brief Register an ensoObject with comms handler and cloud.
 *
 * \param newObject     The object to be registered.
 *
 * \return              EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_RegisterEnsoObjectDirectly(EnsoObject_t* newObject)
{
    if (!newObject)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    prv_SetDeviceStatus(newObject, THING_DISCOVERED);

    // Subscribe Comms Handler to the new object.
    return LSD_SubscribeToDeviceDirectly(&newObject->deviceId, REPORTED_GROUP, COMMS_HANDLER, false);
}

/**
 * \name LSD_FindEnsoObjectByDeviceIdDirectly
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
EnsoObject_t* LSD_FindEnsoObjectByDeviceIdDirectly(const EnsoDeviceId_t* deviceId)
{
    EnsoObject_t* theObject = NULL;

    if (!deviceId)
    {
        return NULL;
    }

    if (!LSD_IsEnsoDeviceIdValid( *deviceId ))
    {
        return NULL;
    }

    for (int i = 0; i <= LSD_MAX_THING ; ++i)
    {
        if ( LSD_IsEnsoDeviceIdValid( prv_ObjectStore[i].deviceId ))
        {
            if (0 == LSD_DeviceIdCompare(deviceId, &(prv_ObjectStore[i].deviceId)))
            {
                theObject = &prv_ObjectStore[i];
                break;
            }
        }
    }

    return theObject;
}



/**
 * \name LSD_FindEnsoObjectNeedingAnnounceDirectly
 *
 * \brief Find any devices that have not been announced to the cloud
 *
 * \return An unannounced object or NULL if none found
 *
 */
EnsoObject_t* LSD_FindEnsoObjectNeedingAnnounceDirectly()
{
    EnsoObject_t* theObject = NULL;

    for (int i = 0; i <= LSD_MAX_THING ; ++i)
    {
        if ( LSD_IsEnsoDeviceIdValid( prv_ObjectStore[i].deviceId ))
        {
            if (!prv_ObjectStore[i].announceAccepted &&
                !prv_ObjectStore[i].announceInProgress)
            {
                /* This object needs announcing so start the announce progress */
                prv_ObjectStore[i].announceInProgress = true;

                theObject = &prv_ObjectStore[i];
                break;
            }
        }
    }

    return theObject;
}

/**
 * \name LSD_GetNotUnsubcribedEnsoObjects
 *
 * \brief Find any devices that could not be unsubscribed
 *
 * \return An could not be unsubscribed object or NULL if none found
 *
 */
EnsoObject_t* LSD_GetNotUnsubcribedEnsoObjects(void)
{
    /* Start from previous objects */
    static int32_t lastObjectId = 0;

    for (int i = 0; i < LSD_MAX_THING; i++)
    {
        lastObjectId = (lastObjectId + 1) % LSD_MAX_THING;

        if (prv_ObjectStore[lastObjectId].unsubscriptionRetryCount > 0)
        {
            return &prv_ObjectStore[lastObjectId];
        }
    }

    return NULL;
}

/**
 * \name LSD_CountEnsoObjectsDirectly
 *
 * \brief Count the number of ensoObjects in the object store
 *
 * \return                  The number of objects in the store
 *
 */
int LSD_CountEnsoObjectsDirectly( void )
{
    int count=0;

    for (int i = 0; i <= LSD_MAX_THING; ++i)
    {
        if ( LSD_IsEnsoDeviceIdValid ( prv_ObjectStore[i].deviceId ))
        {
            count++;
        }
    }

    return count;
}


/*
 * \brief Walk through delta list notify the subscribers.
 *
 * \param   source          The id of the publisher (handler id)
 *
 * \param   destObject      The object which properties are being modified
 *
 * \param   propertyGroup   The group of the property
 *
 * \param   deltas          The block of property agent side identifiers and
 *                          their new values.
 *
 * \param   numProperties   The number of properties in deltas.
 *
 * \return                  ensoError
 */
EnsoErrorCode_e LSD_NotifyDirectly(
        const HandlerId_e source,
        const EnsoObject_t* destObject,
        const PropertyGroup_e propertyGroup,
        const EnsoPropertyDelta_t* deltas,
        const uint16_t numProperties)
{
    if (numProperties > ECOM_MAX_DELTAS)
    {
        return eecBufferTooBig;
    }

    EnsoErrorCode_e retVal = eecNoError;

    /*
     * First, created a new delta array that only contains public properties as we will not want to send updates
     * on private properties to public subscribers
     */
    EnsoPropertyDelta_t adjustedDeltas[ECOM_MAX_DELTAS];
    uint16_t numAdjustedProperties = 0;

    for (unsigned int i = 0; i < numProperties; i++)
    {
        // Find the property and check if it is private
        EnsoProperty_t* theProperty;
        theProperty = LSD_FindPropertyByAgentSideIdDirectly(destObject, deltas[i].agentSidePropertyID);
        if (theProperty)
        {
            if (PROPERTY_PRIVATE != theProperty->type.kind)
            {
                // Add property to the adjusted delta array.
                memcpy(&adjustedDeltas[numAdjustedProperties], &deltas[i], sizeof(EnsoPropertyDelta_t));
                ++numAdjustedProperties;
            }
        }
    }

    /*
     * Second, find the subscribers for the entire object and notify them
     */
    {
        unsigned int subscriberBit = 0;
        while (destObject->subscriptionBitmap[propertyGroup] && subscriberBit < LSD_SUBSCRIBER_BITMAP_SIZE)
        {
            if (IS_BIT_SET(destObject->subscriptionBitmap[propertyGroup], subscriberBit))
            {
                HandlerId_e theSubscriberId;
                bool isSubscriberPrivate;
                retVal = LSD_GetSubscriberIdDirectly(&theSubscriberId, &isSubscriberPrivate, propertyGroup, subscriberBit);
                /* Don't send back the deltas to the publishing object */
                if (eecNoError == retVal && theSubscriberId != source)
                {
                    if (isSubscriberPrivate)
                    {
                        // Send the original deltas to the private subscriber.
                        retVal = ECOM_SendUpdateToSubscriber(
                            theSubscriberId,
                            destObject->deviceId,
                            propertyGroup,
                            numProperties,
                            deltas);
                    }
                    else
                    {
                        // Send the adjusted deltas to the public subscriber.
                        retVal = ECOM_SendUpdateToSubscriber(
                            theSubscriberId,
                            destObject->deviceId,
                            propertyGroup,
                            numAdjustedProperties,
                            adjustedDeltas);
                    }

                    if (eecNoError != retVal)
                    {
                        LOG_Error("ECOM_SendUpdateToSubscriber failed %s", LSD_EnsoErrorCode_eToString(retVal));
                    }
                }
            }
            ++subscriberBit;
        }
    }

    /*
     * Finally, for properties subscribers to any of the ones that have changed, send them a single delta message
     * containing all properties that have changed.
     */
    for (unsigned int subscriberBit = 0; subscriberBit < LSD_SUBSCRIBER_BITMAP_SIZE; subscriberBit += 1)
    {
        HandlerId_e theSubscriberId;
        bool isSubscriberPrivate;
        EnsoErrorCode_e lsdRetVal = LSD_GetSubscriberIdDirectly(&theSubscriberId, &isSubscriberPrivate, propertyGroup, subscriberBit);
        if (lsdRetVal == eecNoError)
        {
            /* See if this subscriber is interested in any of the property deltas. */
            bool isInterested = false;
            for (unsigned int propIdx = 0; propIdx < numProperties; propIdx += 1)
            {
                EnsoProperty_t* theProperty = LSD_FindPropertyByAgentSideIdDirectly(destObject, deltas[propIdx].agentSidePropertyID);
                if (theProperty)
                {
                    /* Only of interest if we're subscribed, and we didn't originate it. */
                    if (IS_BIT_SET(theProperty->subscriptionBitmap[propertyGroup], subscriberBit) && (theSubscriberId != source))
                    {
                        if (PROPERTY_PRIVATE == theProperty->type.kind)
                        {
                            assert(isSubscriberPrivate);
                        }
                        isInterested = true;
                        break;
                    }
                }
                else
                {
                    LOG_Error("LSD_FindPropertyByAgentSideIdDirectly failed for property %d", deltas[propIdx].agentSidePropertyID);
                }
            }

            /* If they are interested, make a single delta message containing only the properties of interest to this subscriber. */
            if (isInterested)
            {
                /* Build a filtered list of the ones of interest to this subscriber. */
                EnsoPropertyDelta_t filteredDeltas[ECOM_MAX_DELTAS];
                uint16_t numFilteredProperties = 0;
                for (unsigned int propIdx = 0; propIdx < numProperties; propIdx += 1)
                {
                    // Find the property and check if it is of interest
                    EnsoProperty_t* theProperty = LSD_FindPropertyByAgentSideIdDirectly(destObject, deltas[propIdx].agentSidePropertyID);
                    if (theProperty)
                    {
                        /* Is this of interest to the subscriber? */
                        if (IS_BIT_SET(theProperty->subscriptionBitmap[propertyGroup], subscriberBit) && (theSubscriberId != source))
                        {
                            if (PROPERTY_PRIVATE == theProperty->type.kind)
                            {
                                assert(isSubscriberPrivate);
                            }

                            memcpy(&filteredDeltas[numFilteredProperties], &deltas[propIdx], sizeof(EnsoPropertyDelta_t));
                            ++numFilteredProperties;
                        }
                    }
                }

                /* Send to the subscriber. */
                retVal = ECOM_SendUpdateToSubscriber(
                    theSubscriberId,
                    destObject->deviceId,
                    propertyGroup,
                    numFilteredProperties,
                    filteredDeltas);

                if (eecNoError != retVal)
                {
                    LOG_Error("ECOM_SendUpdateToSubscriber failed %s", LSD_EnsoErrorCode_eToString(retVal));
                }
            }
        }
    }

    return retVal;
}


/**
 * \name LSD_SetDeviceStatusDirectly
 *
 * \brief Set the device status.
 *
 * \param theDevice        The device identifier
 *
 * \param deviceStatus     The device status
 *
 * \return                A negative number denotes an error
 *                        code
 */
EnsoErrorCode_e LSD_SetDeviceStatusDirectly(
        const EnsoDeviceId_t theDevice,
        const EnsoDeviceStatus_e deviceStatus)
{
    EnsoErrorCode_e retVal = eecNoError;
    EnsoObject_t* theObject = LSD_FindEnsoObjectByDeviceIdDirectly(&theDevice);
    if (!theObject)
    {
        retVal = eecEnsoObjectNotFound;
    }
    else
    {
        prv_SetDeviceStatus(theObject, deviceStatus);
    }

    return retVal;
}


/**
 * \brief   Send out-of-sync properties to a destination handler
 *
 * \param   destination     Where to send the deltas
 *
 * \param   propertyGroup   Property group to collate the changes from
 *
 * \param   maxMessages     Maximum number of message to send
 *
 * \param[out] sentMessages Number of messages sent
 *
 * \return                  EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_SyncLocalShadow(
        const HandlerId_e destination,
        const PropertyGroup_e propertyGroup,
        const uint16_t maxMessages,
        uint16_t* sentMessages)
{
    /* Sanity checks */
    if (!sentMessages)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    EnsoErrorCode_e  retVal = eecNoError;
    uint16_t maxNumMessages = maxMessages;
    uint16_t numMessages = 0;

    *sentMessages = 0;
    int i = 0;
    while (maxNumMessages && i <= LSD_MAX_THING)
    {
        retVal = LSD_SendPropertiesByFilter_Safe(
                PROPERTY_FILTER_OUT_OF_SYNC,
                &prv_ObjectStore[i],
                destination,
                propertyGroup,
                maxNumMessages,
                &numMessages);

        if (eecNoError == retVal && numMessages)
        {
            maxNumMessages -= numMessages;
            *sentMessages += numMessages;
            LOG_Info("Sent out-of-sync deltas using %d messages", numMessages);
        }

        ++i;
    }

    if (i == LSD_MAX_THING)
    {
        if ((DESIRED_GROUP == propertyGroup && !prv_ObjectStore[i].desiredOutOfSync) ||
            (REPORTED_GROUP == propertyGroup && !prv_ObjectStore[i].reportedOutOfSync))
        {
            LOG_Info("Local Shadow sync completed for %s group", LSD_Group2s(propertyGroup));
        }
    }

    return retVal;
}


/**
 * \brief   Send properties which meet the filter condition to a destination handler
 *
 * \param   destination     Where to send the deltas
 *
 * \param   propertyGroup   Property group to collate the values from
 *
 * \param   filter          Persistent only or all properties
 *
 * \param[out] sentDeltas   Number of deltas sent
 *
 * \return                  EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_SendPropertiesByFilter(
        const HandlerId_e destination,
        const PropertyGroup_e propertyGroup,
        const PropertyFilter_e filter,
        uint16_t* sentDeltas)
{
    /* Sanity checks */
    if (!sentDeltas)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }
    if (filter != PROPERTY_FILTER_PERSISTENT && filter != PROPERTY_FILTER_ALL)
    {
        return eecParameterOutOfRange;
    }

    EnsoErrorCode_e retVal = eecNoError;
    uint16_t numDeltas = 0;

    *sentDeltas = 0;
    int i = 0;
    while (i <= LSD_MAX_THING)
    {
        if ((filter == PROPERTY_FILTER_PERSISTENT) &&
            (destination == STORAGE_HANDLER))
        {
            // Producer (us) can generate much faster than consumer (storage) so ensure that
            // there is space in the queue for this device
            MessageQueue_t mq = ECOM_GetMessageQueue(STORAGE_HANDLER);
            int mqSize = OSAL_GetMessageQueueSize(mq);
            // 5 messages will be plenty per device, up to 30 properties
            const int spaceNeeded = 5;
            int space;
            do
            {
                space = mqSize - OSAL_GetMessageQueueNumCurrentMessages(mq);
                if (space < spaceNeeded)
                {
                    LOG_Warning("Sleeping LSD message queue, space in storage queue = %d", space);
                    OSAL_sleep_ms(25);
                }
            }
            while(space < spaceNeeded);
        }

        retVal = LSD_SendPropertiesByFilter_Safe(
                filter,
                &prv_ObjectStore[i],
                destination,
                propertyGroup,
                USHRT_MAX,
                &numDeltas);

        if (eecNoError == retVal && numDeltas)
        {
            *sentDeltas += numDeltas;
            LOG_Info("Sent %d messages", numDeltas);
        }

        ++i;
    }

    // We are done ; notify the destination handler
    retVal = ECOM_SendLocalShadowStatusToSubscriber(destination, LSD_DUMP_COMPLETED, propertyGroup);
    LOG_Info("Local Shadow dump completed for property group %d", propertyGroup);

    return retVal;
}


/**
 * \brief   Get the list of devices that are managed by the handler specified as
 * an argument
 *
 * NOT THREAD SAFE
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
EnsoErrorCode_e LSD_GetDevicesIdDirectly(
        const HandlerId_e handlerId,
        EnsoDeviceId_t* buffer,
        const uint16_t  bufferElems,
        uint16_t* numDevices)
{
    // Sanity check
    if (!buffer || !numDevices)
    {
        LOG_Error("null pointer input param");
        return eecNullPointerSupplied;
    }

    EnsoErrorCode_e  retVal = eecNoError;
    *numDevices = 0;
    for (unsigned int i = 0; i <= LSD_MAX_THING; ++i)
    {
        if (LSD_IsEnsoDeviceIdValid(prv_ObjectStore[i].deviceId))
        {
            EnsoProperty_t* property = LSD_FindPropertyByAgentSideIdDirectly(&prv_ObjectStore[i], PROP_OWNER_ID);
            EnsoPropertyValue_u* ptrValue = LSD_GetPropertyValuePtrFromGroupDirectly(property, REPORTED_GROUP);
            if (ptrValue)
            {
                if (handlerId == ptrValue->uint32Value)
                {
                    if (*numDevices >= bufferElems)
                    {
                        retVal = eecBufferTooSmall;
                        break; // Early exit
                    }
                    buffer[*numDevices] = prv_ObjectStore[i].deviceId;
                    (*numDevices)++;
                }
            }
        }
    }

    return retVal;
}

/**
 * \brief    Dumps the Object Store hierarchy
 */
void LSD_DumpObjectStore(void)
{
    for (unsigned int i = 0; i <= LSD_MAX_THING; i++)
    {
        EnsoObject_t   * p = &prv_ObjectStore[i];
        if (LSD_IsEnsoDeviceIdValid(p->deviceId))
        {
#if 0
            LOG_Info("%3d %016"__PRIx64" %04x %02x %s %s %s %d",
                i, p->deviceId.deviceAddress, p->deviceId.technology, p->deviceId.childDeviceId,
                p->announceAccepted  ? "A" : " ",
                p->reportedOutOfSync ? "R" : " ",
                p->desiredOutOfSync  ? "D" : " ",
                p->propertyListStart);
#endif
            LSD_DumpProperties(p->propertyListStart);
       }
    }
}
