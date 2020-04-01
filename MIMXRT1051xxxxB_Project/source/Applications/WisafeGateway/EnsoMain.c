/*!****************************************************************************
 * \file    EnsoMain.c
 *
 * \author  Evelyne Donnaes
 *
 * \brief   Entry point for WisafeGateway
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "APP_Init.h"
#include "LOG_Api.h"
#include "WiSafe_Main.h"
#include "LSD_Api.h"
#include "wisafe_drv.h"

// Create the AWS connection and start polling
static EnsoDeviceId_t gatewayId;

/**
 * \name   main
 * \brief  WisafeGateway main function
 * \param  argc     - Number of command line arguments
 * \param  argv     - Argument strings
 */
int EnsoMain(int argc, char* argv[])
{
    EnsoErrorCode_e retVal;
    ////////LOG_Init();	//////// [RE:workaround]

//////// [RE:platform]
/*
    // Retrieve cert names
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
    retVal = WiSafe_Start();    //////// [RE:fixme] Some sort of deadlock happens (occasionally) here during connection, needs investigating
    if (eecNoError != retVal)
    {
        LOG_Error("WiSafe_Start error %s", LSD_EnsoErrorCode_eToString(retVal));
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
        LOG_Info(" w - Send custom WiSafe message");
        LOG_Info(" d - Dump object store");

        char buffer[256];
        if (!OSAL_fgets(buffer, sizeof buffer, stdin))
        {
            break; // Early exit
        }
        char c = buffer[0];
        bool b = buffer[1] == '1';
        switch (c)
        {
        case 'd':
            LSD_DumpObjectStore();
            break;
        case 'e':
            LOG_EnableError(b);
            break;
        case 'i':
            LOG_EnableInfo(b);
            break;
        case 'q':
            keepLooping = false;
            break;
        case 't':
            LOG_EnableTrace(b);
            break;
	//////// [RE:wisafe]
        case 'w':
            {
                // support backspace
                {
                    int index=1;
                    while ((buffer[index] != 0)  /*&& (buffer[index + 1] != 0)*/)
                    {
                        if((index>1) && (buffer[index] == 0x08))
                        {
                            strcpy(&buffer[index-1],&buffer[index+1]);
                            index--;
                        }
                        else
                        {
                            index++;
                        }
                    }
                }
                uint8_t bytes[128];
                int bufferIndex = 1;
                int byteIndex = 0;
                while (buffer[bufferIndex] != 0 && buffer[bufferIndex + 1] != 0)
                {
                    char byteString[3];
                    memcpy(byteString, buffer + bufferIndex, 2);
                    byteString[2] = 0;
                    unsigned char byte = (unsigned char)(strtol(byteString, NULL, 16));
                    bytes[byteIndex++] = byte;
                    bufferIndex += 2;
                }
                wisafedrv_write(bytes, byteIndex);
                break;
            }
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
