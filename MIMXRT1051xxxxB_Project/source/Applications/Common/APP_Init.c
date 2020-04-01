/*!****************************************************************************
 *
 * \file APP_Init.c
 *
 * \author Murat Cakmak
 *
 * \brief Application Initialisation Implementation
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "SYS_Gateway.h"
#include "AWS_CommsHandler.h"
#include "ECOM_Api.h"
#include "LEDH_Api.h"
#include "LSD_Api.h"
#include "LOG_Api.h"
#include "UPG_Api.h"
#include "STO_Handler.h"
#include "../Watchdog/Watchdog.h" //nishi
#if TEST_HARNESS
#include "THA_Api.h"
#endif

#include "HAL.h"
#include "APP_Types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*!****************************************************************************
 * Constants
 *****************************************************************************/


/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

/******************************************************************************
 * Public Functions
 *****************************************************************************/
extern EnsoErrorCode_e GW_Initialise(void);

/**
 * \name APP_Initialise
 *
 * \brief Initialise the Application
 *
 * \param[in]  awsIOTRootCAFilename Root CA File Name for AWS IoT
 * \param[in]  awsIOTCertificateFilename Certificate File Name for AWS IoT
 * \param[in]  awsIOTCertificateFilename Certificate File Name for AWS IoT
 * \param[out] gatewayId Gateway ID
 *
 * \return EnsoErrorCode_e
 */
EnsoErrorCode_e APP_Initialise(char* awsIOTRootCAFilename,
                               char* awsIOTCertificateFilename,
                               char* awsIOTPrivateKeyFilename,
                               EnsoDeviceId_t* gatewayId)
{
    EnsoErrorCode_e retVal = eecNoError;
    HAL_Initialise();

    ////////Watchdog_Init(60000);    // div by 3 = 20 sec kicks
    Watchdog_Init(120000);    //////// [RE:workaround] 60 sec sometimes too short to deal with repeatedly failed/delayed connections

    ECOM_Init();
    retVal = LSD_Init();
    if (eecNoError != retVal)
    {
        LOG_Error("Error initialising Local Shadow %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    retVal = SYS_Initialise();
    if (eecNoError != retVal)
    {
        LOG_Error("Error initialising SYS %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    retVal = GW_Initialise();
    if (eecNoError != retVal)
    {
        LOG_Error("Error initialising GW %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    retVal = STO_Handler_Init();
    if (eecNoError != retVal)
    {
        LOG_Error("STO_Handler_Init error %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    retVal = AWS_Initialise(awsIOTRootCAFilename, awsIOTCertificateFilename, awsIOTPrivateKeyFilename);
    if (eecNoError != retVal)
    {
        LOG_Error("Error initialising AWS %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    retVal = LEDH_Initialise();
    if (eecNoError != retVal && eecDeviceNotSupported != retVal)
    {
        LOG_Error("Error initialising LEDH %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    else
    {
        LOG_Info("Device LEDH %s", eecNoError == retVal ? "Initialised" : "Not Installed");
    }
    retVal = UPG_Init();
    if (eecNoError != retVal)
    {
        LOG_Error("Error initialising UPG %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
#if TEST_HARNESS
    retVal = THA_Init();
    if (eecNoError != retVal && eecDeviceNotSupported != retVal)
    {
        LOG_Error("Error initialising THA %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    else
    {
        LOG_Info("Device THA %s", eecNoError == retVal ? "Initialised" : "Not Installed");
    }
    retVal = THA_Start();
    if (eecNoError != retVal && eecDeviceNotSupported != retVal)
    {
        LOG_Error("THA_Start error %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
#endif
    SYS_GetDeviceId(gatewayId);
    retVal = AWS_Start(gatewayId);
    if (eecNoError != retVal)
    {
        LOG_Error("AWS_Start error %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    /*
     * This is only safe to do once we have a certificate and are connected, otherwise
     * we do not recover. This code has been moved into AWS_CommsHandler.
     *
    retVal = GW_Start();
    if (eecNoError != retVal)
    {
        LOG_Error("GW_Start error %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }
    */
    return retVal;
}
