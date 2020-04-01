#ifndef _CLD_COMMS_INTERFACE_H_
#define _CLD_COMMS_INTERFACE_H_

/*****************************************************************************
 *
 * \file CLD_CommsInterface.h
 *
 * \author Carl Pickering
 *
 * \brief An interface to provide an abstraction of the comms interface.
 * It is feasible the communications interface is going to need to be changed,
 * possibly to COAP or possibly the platform to Azure. For this reason it is
 * desirable to allow this module to be switched easily. In C++ this would be
 * done with a class but here in the C world it is more appropriate to define
 * function interfaces for each system we intend to interface to.
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <limits.h>

#include "LSD_Types.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/

#define CLD_COMMS_FILENAMEPATH_MAX (255)


/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

// Forward declarations
struct CLD_CommsChannel_tag;
typedef struct CLD_CommsChannel_tag CLD_CommsChannel_t;

/**
 * \name CommsChannelInitialise_t
 *
 * \brief Defines function pointer for initialising the comms channel.
 *
 * This function is responsible for acquiring (as opposed to allocating) the
 * memory and other resources needed to maintain a communications interface
 * between the ensoAgent and the ensoCloud.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelInitialise_t)(CLD_CommsChannel_t* commsChannel);

/**
 * \name CommsChannelOpen_t
 *
 * \brief Defines function pointer for opening the communications channel.
 *
 * This function is responsible for setting up the communications link between
 * the local shadow and the cloud.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   gatewayId           The gateway identifier used for the Shadow thing Id
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelOpen_t)(CLD_CommsChannel_t* commsChannel, EnsoDeviceId_t* gatewayId);

/**
 * \name CommsChannelDeltaStart_t
 *
 * \brief Defines function pointer for initialising a delta message
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   propertyGroup       The group of the properties in the delta me
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelDeltaStart_t)(CLD_CommsChannel_t* commsChannel, const PropertyGroup_e propertyGroup);

/**
 * \name CommsChannelDeltaAddPropertyStr_t
 *
 * \brief Defines function pointer for adding a string property
 *
 * This function begins the process of creating a delta message ready to be
 * filled using Add before being Sent or Finished.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   propertyString          The property including ID in string form
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelDeltaAddPropertyStr_t)(CLD_CommsChannel_t* commsChannel, const char* propertyString);

/**
 * \name CommsChannelDeltaAddProperty_t
 *
 * \brief Defines function pointer for adding a property object
 *
 * This function begins the process of creating a delta message ready to be
 * filled using Add before being Sent or Finished.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   property                The property to update
 *
 * \param   firstNestedProperty     Add as first nested property
 *
 * \param   group                   Which group are we interested in
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelDeltaAddProperty_t)(CLD_CommsChannel_t* commsChannel,
        const EnsoProperty_t* property, bool firstNestedProperty, PropertyGroup_e group);

/**
 * \name CommsChannelDeltaSend_t
 *
 * \brief Defines function pointer for sending the delta message
 *
 * This function begins the process of creating a delta message ready to be
 * filled using Add before being Sent or Finished.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \param   deviceId            Owning device
 *
 * \param   context             Pointer for a context to be used in callback
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelDeltaSend_t)(CLD_CommsChannel_t* commsChannel,
            const EnsoDeviceId_t deviceId, void* context);

/**
 * \name CommsChannelDeltaFinish_t
 *
 * \brief Defines function pointer for clearing down the delta message
 *
 * This function clears the buffers used by the delta message ready for a new
 * message to be started.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelDeltaFinish_t)(CLD_CommsChannel_t* commsChannel);

/**
 * \name CommsChannelPoll_t
 *
 * \brief Defines the function pointer to perform a single pass of the polling loop.
 *
 * This function polls the communication channel.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelPoll_t)(CLD_CommsChannel_t* commsChannel);

/**
 * \name CommsChannelClose_t
 *
 * \brief Defines function pointer to close down the communications channel
 *
 * This function closes the communication channel.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelClose_t)(CLD_CommsChannel_t* commsChannel);

/**
 * \name CommsChannelDestroy_t
 *
 * \brief Defines function pointer for destroying the communications channel.
 * Free any resources used by the communications channel and make sure
 * everything is cleaned up.
 *
 * \param   commsChannel        Pointer to this structure
 *
 * \return  Error code
 */
typedef EnsoErrorCode_e (*CommsChannelDestroy_t)(CLD_CommsChannel_t* commsChannel);

/**
 * \name SetChannelCertificates_t
 *
 * \brief Defines the function pointer for setting the Certificate Authority,
 * certificate and private key used by the device.
 *
 * \param commsChannel      Pointer to this structure
 *
 * \param rootCAFilename        The name and path of the root certificate authority file.
 *
 * \param certificateFilename   The name and path of the device certificate file.
 *
 * \param privateKeyFilename    The name and path of the device key file.
 *
 * return Error code
 */
typedef EnsoErrorCode_e (*SetChannelCertificates_t)(CLD_CommsChannel_t* commsChannel, const char* rootCAFilename, const char* certificateFilename, const char* privateKeyFilename);


struct CLD_CommsChannel_tag
{
    CommsChannelOpen_t Open;
    CommsChannelDeltaStart_t DeltaStart;
    CommsChannelDeltaAddPropertyStr_t DeltaAddPropertyString;
    CommsChannelDeltaAddProperty_t DeltaAddProperty;
    CommsChannelDeltaSend_t DeltaSend;
    CommsChannelDeltaFinish_t DeltaFinish;
    CommsChannelPoll_t Poll;
    CommsChannelClose_t Close;
    CommsChannelDestroy_t Destroy;

    char rootCertifcateAuthorityFilename[CLD_COMMS_FILENAMEPATH_MAX + 1];
    char clientCertificateFilename[CLD_COMMS_FILENAMEPATH_MAX + 1];
    char clientKeyFilename[CLD_COMMS_FILENAMEPATH_MAX + 1];
};


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e CLD_BaseCommsChannelInitialise(
        CLD_CommsChannel_t* commsChannel,
        const char* rootCAFilename,
        const char* certificateFilename,
        const char* privateKeyFilename);

EnsoErrorCode_e CLD_SetTLSConfiguration(const char* rootCAFilename,
        const char* certificateFilename,
        const char* privateKeyFilename);

#endif
