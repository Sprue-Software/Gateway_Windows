/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_rtwdog.h"
#if defined(FSL_FEATURE_SOC_RCM_COUNT) && (FSL_FEATURE_SOC_RCM_COUNT)
#include "fsl_rcm.h"
#endif
#if defined(FSL_FEATURE_SOC_SMC_COUNT) && (FSL_FEATURE_SOC_SMC_COUNT > 1)
#include "fsl_msmc.h"
#endif
#if defined(FSL_FEATURE_SOC_ASMC_COUNT) && (FSL_FEATURE_SOC_ASMC_COUNT)
#include "fsl_asmc.h"
#endif
#include "pin_mux.h"
#include "clock_config.h"
#include "wisafe_main.h"
#include "radio.h"

#include "LOG_Api.h"



/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* RESET_CHECK_FLAG is a RAM variable used for wdog32 self test.
 * Make sure this variable's location is proper that it will not be affected by watchdog reset,
 * that is, the variable shall not be intialized in startup code.
 */
#define RESET_CHECK_FLAG (*((uint32_t *)0x20001000))
#define RESET_CHECK_INIT_VALUE 0x0D0D
#define EXAMPLE_WDOG_BASE RTWDOG
#define WDOG_IRQHandler RTWDOG_IRQHandler

#define WDOG_WCT_INSTRUCITON_COUNT (128U)



/*******************************************************************************
 * Prototypes
 ******************************************************************************/



/*******************************************************************************
 * Variables
 ******************************************************************************/

static RTWDOG_Type *rtwdog_base = EXAMPLE_WDOG_BASE;
#if defined(FSL_FEATURE_SOC_RCM_COUNT) && (FSL_FEATURE_SOC_RCM_COUNT)
static RCM_Type *rcm_base = RCM;
#endif
static rtwdog_config_t config;

static uint32_t wisafe_main_loop_count = 0;



/*******************************************************************************
 * Code
 ******************************************************************************/

#if !(defined(FSL_FEATURE_SOC_ASMC_COUNT) && (FSL_FEATURE_SOC_ASMC_COUNT))
/*!
 * @brief WDOG0 IRQ handler.
 *
 */
void WDOG_IRQHandler(void)
{
    RTWDOG_ClearStatusFlags(rtwdog_base, kRTWDOG_InterruptFlag);
    
    RESET_CHECK_FLAG++;
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}
#endif /* FSL_FEATURE_SOC_ASMC_COUNT */



void watchdog_init(uint32_t timeout_ms)
{
    // Convert time
    uint32_t freq = CLOCK_GetRtcFreq() / 256;   // 256 is our divider
    uint32_t timeoutTicks = freq * timeout_ms / 1000;

    // Configure wdog
    /*
    config.enableWdog32 = true;
    config.clockSource = kWDOG32_ClockSource1;
    config.prescaler = kWDOG32_ClockPrescalerDivide1;
    config.testMode = kWDOG32_TestModeDisabled;
    config.enableUpdate = true;
    config.enableInterrupt = false;
    config.enableWindowMode = false;
    config.windowValue = 0U;
    config.timeoutValue = 0xFFFFU;
    */

    RTWDOG_GetDefaultConfig(&config);

    config.clockSource = kRTWDOG_ClockSource1;
    config.prescaler = kRTWDOG_ClockPrescalerDivide256;
    config.timeoutValue = timeoutTicks;
}



void watchdog_start()
{
    // Enable RTWDOG clock
    CLOCK_EnableClock(kCLOCK_Wdog3);
    NVIC_EnableIRQ(RTWDOG_IRQn);

#if defined(FSL_FEATURE_SOC_ASMC_COUNT) && (FSL_FEATURE_SOC_ASMC_COUNT)    
    if ((ASMC_GetSystemResetStatusFlags(EXAMPLE_ASMC_BASE) & kASMC_WatchdogResetFlag))
    {
        RESET_CHECK_FLAG++;
    }  
#endif
   
    RTWDOG_Init(rtwdog_base, &config);
}



void watchdog_refresh()
{
    uint32_t new_loop_count = getWisafeMainLoopCount();

    if (new_loop_count != wisafe_main_loop_count)
    {
        wisafe_main_loop_count = new_loop_count;
        RTWDOG_Refresh(rtwdog_base);

    }
    else
    {
        LOG_Warning("Wisafe main loop has not run since last watchdog check !!!");
#if ENHANCED_DEBUG
        LOG_Warning("History:");
        for (int i=0 ; i < sizeof(WiSafe_RM_Task_Point) ; i++)
        {
            LOG_Warning("%02d: 0x%02x",i-sizeof(WiSafe_RM_Task_Point),WiSafe_RM_Task_Point[(WiSafe_RM_Task_Point_Index+i)%sizeof(WiSafe_RM_Task_Point)]);
        }
#endif
    }
}
