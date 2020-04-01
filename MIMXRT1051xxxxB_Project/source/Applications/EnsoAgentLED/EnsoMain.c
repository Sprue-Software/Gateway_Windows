/*!****************************************************************************
 * \file    EnsoMain.c
 *
 * \author  Evelyne Donnaes
 *
 * \brief   Entry point for EnsoAgentLED
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include "APP_Init.h"
#include "LOG_Api.h"
#include "LSD_Api.h"


// Create the AWS connection and start polling
static EnsoDeviceId_t gatewayId;

/**
 * \name   main
 * \brief  EnsoAgentLED main function
 * \param  argc     - Number of command line arguments
 * \param  argv     - Argument strings
 */
//////// [RE:platform]
////////#ifdef Ameba
int EnsoMain(int argc, char* argv[])
////////#else
////////int main(int argc, char* argv[])
////////#endif
{
    EnsoErrorCode_e retVal;
    ////////LOG_Init();	//////// [RE:workaround]

//////// [RE:platform]
/*
    char * aws_iot_root_ca_filename     = getenv("AWS_IOT_ROOT_CA_FILENAME");
    char * aws_iot_certificate_filename = getenv("AWS_IOT_CERTIFICATE_FILENAME");
    char * aws_iot_private_key_filename = getenv("AWS_IOT_PRIVATE_KEY_FILENAME");
    if (aws_iot_root_ca_filename == NULL     ||
        aws_iot_certificate_filename == NULL ||
        aws_iot_private_key_filename == NULL)
    {
        if ( argc >= 4 )
        {
            aws_iot_root_ca_filename     = argv[1];
            aws_iot_certificate_filename = argv[2];
            aws_iot_private_key_filename = argv[3];
        }
        else
        {
            LOG_Error("Usage %s rootCA Cert Key\n", argv[0]);
            return -1;
        }
    }
*/
    // Retrieve cert names
    char * aws_iot_root_ca_filename     = argv[1];
    char * aws_iot_certificate_filename = argv[2];
    char * aws_iot_private_key_filename = argv[3];

    // Basic Initialisations
    retVal = APP_Initialise(aws_iot_root_ca_filename, aws_iot_certificate_filename, aws_iot_private_key_filename, &gatewayId);
    if (eecNoError != retVal)
    {
        LOG_Error("APP Initialisation error %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }

    //------------------------------------------------------------------------
    /// Main event loop
    //------------------------------------------------------------------------
    bool keepLooping = true;
    do
    {
        LOG_Info("Commands:");
        LOG_Info(" q - Quit application");
        LOG_Info(" d - Dump object store");

        char buffer[64];
        if (!OSAL_fgets(buffer, sizeof buffer, stdin))
        {
            break; // Early exit
        }
        char c = buffer[0];
        switch (c)
        {
        case 'd':
            LSD_DumpObjectStore();
            break;
        case 'q':
            keepLooping = false;
            break;
        default:
            break;
        }
    }
    while (keepLooping);

    //------------------------------------------------------------------------
    /// Clean-up
    //------------------------------------------------------------------------

    return 0;
}
