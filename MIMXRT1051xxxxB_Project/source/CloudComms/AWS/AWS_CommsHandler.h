#ifndef _AWS_COMMS_HANDLER_H_
#define _AWS_COMMS_HANDLER_H_

/*!****************************************************************************
*
* \file AWS_CommsHandler.h
*
* \author Carl Pickering
*
* \brief A shim to allow for the AWS communications to be abstracted out.
* It is anticipated this shim will be replaced with different shims if we wish
* to use other protocols such as COAP or alternative platforms such as Azure.
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include "LSD_Types.h"
#include "CLD_CommsInterface.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/

/*
 * Maximum Subscription Count per channel
 *
 * AWS SDK allows only 50 subscription per device but both AWS SDK and
 * Comms_Hander modules keep their subscription counts.
 * For non-persistent subscriptions AWS SDK subscribes to/unsubscribes from
 * two topic internally so it can cause an oscillation and if this oscillation
 * hit a corner case for new device description (~49th device), AWS refuses
 * request. See G2-3059.
 * To avoid this issue lets keep a margin for a channel.
 */
#define MAX_SUBSCRIPTION_PER_MQTT_CHANNEL   47

#define MAX_CONNECTION_PER_GATEWAY          5

#define MAX_SUBSCRIPTION_PER_GATEWAY        (MAX_CONNECTION_PER_GATEWAY * MAX_SUBSCRIPTION_PER_MQTT_CHANNEL)



/******************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e AWS_Initialise(
        char* rootCAFileName,
        char* clientCertificateFileName,
        char* clientKeyFileName);

EnsoErrorCode_e AWS_Start(EnsoDeviceId_t* gatewayId);

void AWS_Stop(void);

bool AWS_OutOfSyncTimerEnabled();

void AWS_StartOutOfSyncTimer(void);

EnsoErrorCode_e AWS_CommsChannelOpen(CLD_CommsChannel_t* commsChannel, EnsoDeviceId_t* gatewayId);

EnsoErrorCode_e AWS_CommsChannelDeltaStart( CLD_CommsChannel_t* commsChannel, const PropertyGroup_e propertyGroup);

EnsoErrorCode_e AWS_CommsChannelDeltaAddPropertyStr( CLD_CommsChannel_t* commsChannel, const char* propertyString);

EnsoErrorCode_e AWS_CommsChannelDeltaAddProperty( CLD_CommsChannel_t* commsChannel,
        const EnsoProperty_t* property, bool firstNestedProperty, PropertyGroup_e group);

EnsoErrorCode_e AWS_CommsChannelDeltaSend( CLD_CommsChannel_t* commsChannel, const EnsoDeviceId_t deviceId,
        void* context);

EnsoErrorCode_e AWS_CommsChannelDeltaFinish( CLD_CommsChannel_t* commsChannel);

EnsoErrorCode_e AWS_CommsChannelPoll(CLD_CommsChannel_t* commsChannel);

EnsoErrorCode_e AWS_CommsChannelClose(CLD_CommsChannel_t* commsChannel);

EnsoErrorCode_e AWS_CommsChannelDestroy(CLD_CommsChannel_t* commsChannel);

EnsoErrorCode_e AWS_OnCommsHandler(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t deviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer,
        void* context);

bool AWS_GetHeadOfDeleteQueue(EnsoDeviceId_t* device);



#endif
