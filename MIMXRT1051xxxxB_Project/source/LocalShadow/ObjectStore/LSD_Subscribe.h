#ifndef __LSD_SUBSCRIBE
#define __LSD_SUBSCRIBE

/*!****************************************************************************
*
* \file LSD_Subscribe.h
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

#include "LSD_EnsoObjectStore.h"

/*!****************************************************************************
 * Macro Definitions
 *****************************************************************************/

#define IS_BIT_SET(var, pos) ((var) & (1<<(pos)))
#define SET_BIT(var, pos)    ((var) |= (1 << (pos)))
#define CLR_BIT(var, pos)    ((var) &= ~((1) << (pos)))

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * Handler type
 */
typedef struct
{
    int  handlerId;
    bool isPrivate;
} HandlerType_t;

#define LSD_SUBSCRIBER_BITMAP_SIZE (32)
extern HandlerType_t LSD_SubscriberArray[PROPERTY_GROUP_MAX][LSD_SUBSCRIBER_BITMAP_SIZE];

void LSD_SubscribeInit(void);

EnsoErrorCode_e LSD_SubscribeToDeviceDirectly(
        const EnsoDeviceId_t* subjectDeviceId,
        PropertyGroup_e subjectPropertyGroup,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate);

EnsoErrorCode_e LSD_SubscribeToDevicePropertyByAgentSideIdDirectly(
        const EnsoDeviceId_t* subjectDeviceId,
        EnsoAgentSidePropertyId_t subjectAgentPropId,
        PropertyGroup_e subjectPropertyGroup,
        const HandlerId_e subscriberId,
        const bool isSubscriberPrivate);

EnsoErrorCode_e LSD_GetSubscriberIdDirectly(
        HandlerId_e* theId,
        bool* isSubscriberPrivate,
        const PropertyGroup_e groupNum,
        const unsigned int subscriberIndex);

#endif
