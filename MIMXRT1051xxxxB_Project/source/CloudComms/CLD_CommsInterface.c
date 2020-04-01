
/*!****************************************************************************
*
* \file CLD_CommsInterface.c
*
* \author Carl Pickering
*
* \brief The basic interface for the comms handler.
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "LSD_Types.h"
#include "CLD_CommsInterface.h"
#include "LOG_Api.h"


/**
 * \name CLD_BaseCommsChannelInitialise
 *
 * \brief Initialise the comms channel with the Certificate Authority,
 * certificate and private key used by the device.
 *
 * \param commsChannel          The comms channel
 *
 * \param rootCAFilename        The name and path of the root certificate authority file
 *
 * \param certificateFilename   The name and path of the device certificate file
 *
 * \param privateKeyFilename    The name and path of the device key file
 *
 * return                       eecNoError on success or the error code
 */
EnsoErrorCode_e CLD_BaseCommsChannelInitialise(
        CLD_CommsChannel_t* commsChannel,
        const char* rootCA,
        const char* certs,
        const char* key)
{
    EnsoErrorCode_e retVal = eecBufferTooSmall;
    /* Sanity check */
    if (!commsChannel || !rootCA || !certs || !key)
    {
        return eecNullPointerSupplied;
    }
    if (snprintf(commsChannel->rootCertifcateAuthorityFilename, CLD_COMMS_FILENAMEPATH_MAX, "%s", rootCA) < CLD_COMMS_FILENAMEPATH_MAX)
    {
        if (snprintf(commsChannel->clientCertificateFilename, CLD_COMMS_FILENAMEPATH_MAX, "%s", certs) < CLD_COMMS_FILENAMEPATH_MAX)
        {
            if (snprintf(commsChannel->clientKeyFilename, CLD_COMMS_FILENAMEPATH_MAX, "%s", key ) < CLD_COMMS_FILENAMEPATH_MAX)
            {
                retVal = eecNoError;
            }
        }
    }
    return retVal;
}


/**
 * \name CLD_SetTLSConfiguration
 *
 * \return                          Error code.
 */
EnsoErrorCode_e CLD_SetTLSConfiguration(
        const char* rootCAFilename,
        const char* certificateFilename,
        const char* privateKeyFilename)
{
    EnsoErrorCode_e retVal = eecNoError;

    /* sanity checks */

    return retVal;
}
