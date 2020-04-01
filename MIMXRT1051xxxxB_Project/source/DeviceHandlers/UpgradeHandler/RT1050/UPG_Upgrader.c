/*!****************************************************************************
 *
 * \file UPG_Upgrader.c
 *
 * \author Murat Cakmak
 *
 * \brief Upgrader Implementation for Ameba Platform
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/


/***************************** INCLUDES ***************************************/
#include "UPG_Upgrader.h"
#include "LOG_Api.h"

#include <stdio.h>
#include <string.h>

/************************* DEFINES & MACROS ***********************************/
#define ENCRYPTED_OTA                   0

#define HTTPS_PORT                      443


/************************* FUNCTION PROTOTYPE *********************************/

extern void AWS_Stop(void);
extern void HAL_Reboot(void);

extern int https_update_ota(char *host, int port, char *resource);
extern int https_ram_update_ota(char *host, int port, char *resource, char* destBuffer, int destBufferSize);

/************************** GLOBAL VARIABLES **********************************/
#if ENCRYPTED_OTA == 1
char upgradeBuffer[1024 * 1024];
#endif


/****************************** FUNCTIONS *************************************/

/**
 * \brief   Upgrade Upgrader - actually does the application upgrade
 *
 * \param   url  URL of file to download
 * \param   key  64 hex digit representation of crypto key
 * \param   hash 64 hex digit representation of file hash
 *
 * \return  0 on success, -ve on failure.
 */
int UPG_Upgrader(char * url, char * key, char * hash)
{
    /*
     * AWS and Upgrade Staff use mbedtls on Ameba which causes race
     * condition.
     * AWS is used to send progress in Linux build but Linux staff uses
     * different ssl stacks for upgrade so no race condition.
     * To avoid race condition in Ameba side lets stop AWS.
     */
    AWS_Stop();

    LOG_Warning("New Firmare Upgrade from %s", url);

    OSAL_sleep_ms(2000);

    char* httpsPrefix = "https://";

    if (strstr(url, httpsPrefix) != NULL)
    {

        /* Remove "https://" if exist */
        url += strlen(httpsPrefix);
    }

    /*
     * URL contains host address (IP) and resource (Image) path but OTA API
     * needs than separately
     */
    char* seperatorPtr;
    if ((seperatorPtr = strchr(url, '/')) == NULL)
    {
        LOG_Error("URL Parse Error.");
        return -1;
    }

    /* Get resource path */
    char* resourceName = seperatorPtr + 1;

    /* Divide url string into two different string (Host and Resource Path) */
    *seperatorPtr = '\0';

#if ENCRYPTED_OTA == 0
    int ret = https_update_ota(url, HTTPS_PORT, resourceName);
#else
    #error "Not fully implemented! Works without image verification!"
    /*
     * https_update_encrpted_ota_on_ram() function is implemented for encrypted
     * OTA updates.
     * It
     *  1 - Collects Encrypted data in RAM (upgradeBuffer) first using HTTPS (Secure)
     *  2 - Checks checksum
     *
     *  3 - [MISSING Implementation!!!] It needs to verify image and encrypt image.
     *
     *  4 - Writes image to flash.
     *  5 - Modifies A/B images to run jump to latest image.
     */
    int ret = https_update_encrpted_ota_on_ram(url, HTTPS_PORT, resourceName, upgradeBuffer, sizeof(upgradeBuffer));
#endif

    if (ret != 0)
    {
        LOG_Error("OTA Upgrade Failed! Error code : %d", ret);
        return -1;
    }

    LOG_Info("OTA Upgrade Success!", ret);
    LOG_Info("Rebooting!", ret);

    OSAL_sleep_ms(1000);

  HAL_Reboot();

    return 0;
}
