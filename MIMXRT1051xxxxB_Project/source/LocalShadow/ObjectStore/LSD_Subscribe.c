/*!****************************************************************************
 *
 * \file LSD_Subscribe.c
 *
 * \author Carl Pickering
 *
 * \brief This module provides the subscription services used in the local
 * shadow.

 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "LSD_EnsoObjectStore.h"
#include "LSD_PropertyStore.h"
#include "LSD_Subscribe.h"
#include "LSD_Api.h"
#include "LOG_Api.h"


/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

/**
 * \name LSD_SubscriberArray
 *
 * \brief The list of subscribers for the whole system. For each property group,
 *        desired or reported, up to 32 subscribers can be registered.
 *        The array provides a mapping between the position in the bitmap and
 *        the handler identifier.
 */
HandlerType_t LSD_SubscriberArray[PROPERTY_GROUP_MAX][LSD_SUBSCRIBER_BITMAP_SIZE];


/*!****************************************************************************
 * Private Functions
 *****************************************************************************/
static EnsoErrorCode_e _AddSubscriber(
        PropertyGroup_e subjectPropertyGroup,
        SubscriptionBitmap_t* subscriptionBitmap,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate);


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/*
 * \brief Initialise the subscribe module
 *
 */
void LSD_SubscribeInit(void)
{
    LOG_Info("Subscriber array reset");
    for (int prop = 0; prop < PROPERTY_GROUP_MAX; prop++)
    {
        for (int i = 0; i < LSD_SUBSCRIBER_BITMAP_SIZE; i++)
        {
            LSD_SubscriberArray[prop][i].handlerId = -1;
            LSD_SubscriberArray[prop][i].isPrivate = 0;
        }
    }
}

/**
 * \name LSD_SubscribeToDeviceDirectly
 *
 * \brief This function is used to subscribe to all properties in a
 *        property group on an ensoObject.
 *
 * \param   subjectDeviceId          The identifier of the thing to subscribe to
 *
 * \param   subjectPropertyGroup    The group to which the properties belongs to
 *
 * \param   subscriberId            The identifier of the subscriber
 *
 * \param   isSubscriberPrivate     Whether the subscriber is private
 *
 * \return                          EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_SubscribeToDeviceDirectly(
        const EnsoDeviceId_t* subjectDeviceId,
        PropertyGroup_e subjectPropertyGroup,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate)
{
    EnsoErrorCode_e retVal = eecNoError;

    if (!subjectDeviceId)
    {
        return eecNullPointerSupplied;
    }
    if (subjectPropertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecParameterOutOfRange;
    }

    EnsoObject_t* subjectObject = LSD_FindEnsoObjectByDeviceIdDirectly(subjectDeviceId);
    if (subjectObject)
    {
        SubscriptionBitmap_t* subscriptionBitmap;
        subscriptionBitmap = &subjectObject->subscriptionBitmap[subjectPropertyGroup];
        retVal = _AddSubscriber(subjectPropertyGroup, subscriptionBitmap, subscriberId, isSubscriberPrivate);
    }
    else
    {
        LOG_Error("Could not find object for thing %ld", subjectDeviceId->deviceAddress);
        retVal = eecEnsoDeviceNotFound;
    }

    return retVal;
}


/**
 * \name LSD_SubscribeToDevicePropertyByAgentSideIdDirectly
 *
 * \brief This function is used to subscribe to a particular property in a
 * property group on an ensoObject.
 *
 * It looks for the subscriber in the subscriber list using its
 * DeviceIdentifer to find out if it's already in there. If not and there's
 * space it is added. If there's no space the subscriber isn't added and an
 * error is returned.
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
 * \param   subscriberId            The identifier of the handler that is trying
 *                                  to subscribe to the updates.
 *
 * \param   isSubscriberPrivate     Whether the subscriber is private
 *
 * \return                          EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_SubscribeToDevicePropertyByAgentSideIdDirectly(
        const EnsoDeviceId_t* subjectDeviceId,
        EnsoAgentSidePropertyId_t subjectAgentPropId,
        PropertyGroup_e subjectPropertyGroup,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate)
{
    EnsoErrorCode_e retVal = eecNoError;

    if (!subjectDeviceId)
    {
        return eecNullPointerSupplied;
    }
    if (subjectPropertyGroup >= PROPERTY_GROUP_MAX)
    {
        return eecParameterOutOfRange;
    }
    if (0 == subjectAgentPropId)
    {
        return eecParameterOutOfRange;
    }

    EnsoObject_t* subjectObject = LSD_FindEnsoObjectByDeviceIdDirectly(subjectDeviceId);
    if (subjectObject)
    {
        EnsoProperty_t* property = LSD_FindPropertyByAgentSideIdDirectly(subjectObject, subjectAgentPropId);
        if (property)
        {
            if (PROPERTY_PRIVATE == property->type.kind && !isSubscriberPrivate)
            {
                LOG_Error("Public subscriber not allowed for private property %x", subjectAgentPropId);
                retVal = eecPropertyWrongType;
            }
            else
            {
                SubscriptionBitmap_t* subscriptionBitmap;
                subscriptionBitmap = &property->subscriptionBitmap[subjectPropertyGroup];
                retVal = _AddSubscriber(subjectPropertyGroup, subscriptionBitmap, subscriberId, isSubscriberPrivate);
            }
        }
        else
        {
            LOG_Error("Could not find property %x", subjectAgentPropId);
            retVal = eecPropertyNotFound;
        }
    }
    else
    {
        LOG_Error("Could not find object for thing %ld", subjectDeviceId->deviceAddress);
        retVal = eecEnsoDeviceNotFound;
    }

    return retVal;
}

/**
 * \name LSD_GetSubscriberIdDirectly
 *
 * \brief Retrieves the HandlerId_e for a subscriber of given group and given index
 *
 * \param[out]  theId                The handler id.
 *
 * \param[out]  isSubscriberPrivate  Whether the subscriber is a private handler.
 *
 * \param       groupNum             The property group of the subscriber.
 *
 * \param       subscriberIndex      The index of the subscriber in the bitmap.
 *
 * \return                           EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_GetSubscriberIdDirectly(
        HandlerId_e* theId,
        bool* isSubscriberPrivate,
        const PropertyGroup_e groupNum,
        const unsigned int subscriberIndex)
{
    /* sanity checks */
    if ( NULL == theId)
    {
        return eecNullPointerSupplied;
    }
    if (groupNum >= PROPERTY_GROUP_MAX)
    {
        return eecParameterOutOfRange;
    }
    if (subscriberIndex >= LSD_SUBSCRIBER_BITMAP_SIZE)
    {
        return eecParameterOutOfRange;
    }

    EnsoErrorCode_e retVal = eecNoError;
    if (LSD_SubscriberArray[groupNum][subscriberIndex].handlerId != -1)
    {
        *theId = LSD_SubscriberArray[groupNum][subscriberIndex].handlerId;
        *isSubscriberPrivate = LSD_SubscriberArray[groupNum][subscriberIndex].isPrivate;
    }
    else
    {
        retVal = eecSubscriberNotFound;
    }

    return retVal;
}

/**
 * \brief This function adds a subscriber to the subscription bit map supplied.
 *
 * It looks for the subscriber in the subscriber list using its
 * subscriberId to find out if it's already in there. If not and there's
 * space it is added. If there's no space the subscriber isn't added and an
 * error is returned.
 *
 * \param   subjectPropertyGroup    The property group to which the
 *                                  subscription bit map belongs to
 *
 * \param   subscriptionBitmap      The subscription bit map
 *
 * \param   subscriberId            The identifier of the subscriber
 *
 * \param   isSubscriberPrivate     Whether the subscriber is private
 *
 * \return                          EnsoErrorCode_e
 *
 */
static EnsoErrorCode_e _AddSubscriber(
        PropertyGroup_e subjectPropertyGroup,
        SubscriptionBitmap_t* subscriptionBitmap,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate)
{
    EnsoErrorCode_e retVal = eecNoError;
    int subscriberBit = -1;

    assert (subscriptionBitmap);

    /* Look for subscriber in subscriber array. */
    for (int i = 0; i < LSD_SUBSCRIBER_BITMAP_SIZE; i++)
    {
        if (LSD_SubscriberArray[subjectPropertyGroup][i].handlerId != -1)
        {
            if (subscriberId == LSD_SubscriberArray[subjectPropertyGroup][i].handlerId)
            {
                /* We have a match */
                subscriberBit = i;
                break;
            }
        }
        else
        {
            /* We have a blank space, which means we're not in the list */
            LSD_SubscriberArray[subjectPropertyGroup][i].handlerId = subscriberId;
            LSD_SubscriberArray[subjectPropertyGroup][i].isPrivate = isSubscriberPrivate;
            subscriberBit = i;
            break;
        }
    }

    if (subscriberBit >= 0)
    {
        /*
         * Add the subscriber to the bitmask
         * As it's an OR it doesn't matter if it's already allocated
         */
        *subscriptionBitmap |= (1 << subscriberBit);
    }
    else
    {
        /* We must have run out of space to get here */
        LOG_Error("Unable to add subscriber %d to bit map %x, out of space", subscriberId, *subscriptionBitmap);
        retVal = eecPoolFull;
    }

    return retVal;
}
