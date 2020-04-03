/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "board.h"
#include "ksdk_mbedtls.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "WF200_host_common.h"


/*******************************************************************************
 * DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES
 ******************************************************************************/

// WF200 Host Control Info
extern eCommsMethod CommsMethod;

// Network Connection Details
extern char WLAN_SSID[20];
extern int  WLAN_SSID_Length;
extern char WLAN_Password[20];
extern int  WLAN_PasswordLength;
extern sl_wfx_security_mode_t WLAN_SecurityMode;


/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 * @brief
 *
 *
 * @param[in]
 *
 * @return
 *    None
 ******************************************************************************/
void BOARD_InitModuleClock(void)
{
    const clock_enet_pll_config_t config = {.enableClkOutput = true, .enableClkOutput25M = false, .loopDivider = 1};
    CLOCK_InitEnetPll(&config);
}


/*******************************************************************************
 * @brief
 *
 *
 * @param[in]
 *
 * @return
 *    None
 ******************************************************************************/
int main(void)
{
    sl_status_t WF200_DriverStatus;

    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    BOARD_InitModuleClock();

    /* Data cache must be temporarily disabled to be able to use sdram */
    SCB_DisableDCache();

    CRYPTO_InitHardware();

    /* Create tcp_ip stack thread */
    tcpip_init(NULL, NULL);

    // WF200 Comms setup
    WF200_DriverStatus = WF200_CommsSetup();

    if(WF200_DriverStatus == SL_STATUS_OK)
    {
    	// The following variables/settings will have
    	// been initialised in the CommsSetup call
    	// above. However the INIT TASK has not started
    	// yet, so the settings can be set below for
    	// when the INIT TASK is started

    	// CommsMethod = LTE;
        CommsMethod = WiFi;

        WF200_SetAwake(false);

        // WF200_Set_AP_ModeRequired(false);
        WF200_Set_AP_ModeRequired(true);

        WF200_Set_STA_ModeRequired(false);
       // WF200_Set_STA_ModeRequired(true);

        WF200_SetTestingModeRequired(false);
        // WF200_SetTestingModeRequired(true);


        // TO BE REMOVED  //////////////////////////////
        // Need to put in the correct details here
        uint8_t tempSSID[] = {'T', 'E', 'S', 'T', 'P', '0', '3', '0', '2'};
        uint8_t tempPass[] = {'T', 'E', 'S', 'T', 'P', '0', '3', '0', '2'};
        WLAN_SecurityMode = WFM_SECURITY_MODE_WPA2_PSK;

        WLAN_SSID_Length = 9;
		WLAN_PasswordLength = 9;
        memcpy(&WLAN_SSID[0], &tempSSID[0], 9);
        memcpy(&WLAN_Password[0], &tempPass[0], 9);
        ////////////////////////////////////////////////
    }
    else
    {
    	// Log failure
	    PRINTF("WF200 Comms Setup: %i\n", WF200_DriverStatus);
	    WF200_DEBUG_TRAP;
    }


    /* Run RTOS */
    vTaskStartScheduler();

    /* Should not reach this statement */
    for (;;)
        ;
}