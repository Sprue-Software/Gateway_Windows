/*
 * Copyright 2017 NXP
 * All rights reserved.
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

#ifdef FSL_RTOS_FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

#include "lpm.h"
#include "fsl_gpc.h"
#include "fsl_dcdc.h"
#include "fsl_gpt.h"

#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LPM_GPC_IMR_NUM (sizeof(GPC->IMR) / sizeof(GPC->IMR[0]))

#ifdef FSL_RTOS_FREE_RTOS
/* Define the counter clock of the systick (GPT). For accuracy purpose,
 * please make LPM_SYSTICK_COUNTER_FREQ divisible by 32768, and times of
 * configTICK_RATE_HZ.
 */
#define LPM_SYSTICK_COUNTER_FREQ (32768)
#define LPM_COUNT_PER_TICK (LPM_SYSTICK_COUNTER_FREQ / configTICK_RATE_HZ)

struct _lpm_power_mode_listener
{
    lpm_power_mode_callback_t callback;
    void *data;
    struct _lpm_power_mode_listener *next;
};

typedef struct _lpm_power_mode_listener lpm_power_mode_listener_t;
#endif

typedef struct _lpm_clock_context
{
    uint32_t armDiv;
    uint32_t ahbDiv;
    uint32_t ipgDiv;
    uint32_t perDiv;
    uint32_t perSel;
    uint32_t periphSel;
    uint32_t preperiphSel;
    uint32_t pfd480;
    uint32_t pfd528;
    uint32_t pllarm_loopdiv;
    uint32_t pllarm;
    uint32_t pllSys;
    uint32_t pllUsb1;
    uint32_t pllUsb2;
    uint32_t pllAudio;
    uint32_t pllVideo;
    uint32_t pllEnet;
    uint32_t is_valid;
} lpm_clock_context_t;

typedef void (*lpm_system_func_t)(uint32_t context);
typedef void (*freertos_tick_func_t)(void);

static lpm_clock_context_t s_clockContext;

#ifdef FSL_RTOS_FREE_RTOS
#if (configUSE_TICKLESS_IDLE == 1)
GPT_Type *vPortGetGptBase(void);
IRQn_Type vPortGetGptIrqn(void);
#endif
#endif


/*******************************************************************************
 * Code
 ******************************************************************************/

/*
 * ERR007265: CCM: When improper low-power sequence is used,
 * the SoC enters low power mode before the ARM core executes WFI.
 *
 * Software workaround:
 * 1) Software should trigger IRQ #32 (IOMUX) to be always pending
 *    by setting IOMUX_GPR1_GINT.
 * 2) Software should then unmask IRQ #32 in GPC before setting CCM
 *    Low-Power mode.
 * 3) Software should mask IRQ #32 right after CCM Low-Power mode
 *    is set (set bits 0-1 of CCM_CLPCR).
 *
 * Note that IRQ #32 is GIC SPI #0.
 */
void LPM_DisablePLLs(lpm_power_mode_t power_mode)
{
    /* Application shouldn't rely on PLL in low power mode,
     * gate all PLL and PFD now */
    if (LPM_PowerModeSuspend == power_mode)
    {
        return;
    }

    s_clockContext.pfd480 = CCM_ANALOG->PFD_480;
    s_clockContext.pfd528 = CCM_ANALOG->PFD_528;
    s_clockContext.pllSys = CCM_ANALOG->PLL_SYS;
    s_clockContext.pllUsb1 = CCM_ANALOG->PLL_USB1;
    s_clockContext.pllUsb2 = CCM_ANALOG->PLL_USB2;
    s_clockContext.pllAudio = CCM_ANALOG->PLL_AUDIO;
    s_clockContext.pllVideo = CCM_ANALOG->PLL_VIDEO;
    s_clockContext.pllEnet = CCM_ANALOG->PLL_ENET;
    s_clockContext.pllarm_loopdiv = (CCM_ANALOG->PLL_ARM & CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK) >> CCM_ANALOG_PLL_ARM_DIV_SELECT_SHIFT;
    s_clockContext.pllarm = CCM_ANALOG->PLL_ARM;
    s_clockContext.periphSel = CLOCK_GetMux(kCLOCK_PeriphMux);
    s_clockContext.ipgDiv = CLOCK_GetDiv(kCLOCK_IpgDiv);
    s_clockContext.ahbDiv = CLOCK_GetDiv(kCLOCK_AhbDiv);
    s_clockContext.perSel = CLOCK_GetMux(kCLOCK_PerclkMux);
    s_clockContext.perDiv = CLOCK_GetDiv(kCLOCK_PerclkDiv);
    s_clockContext.preperiphSel = CLOCK_GetMux(kCLOCK_PrePeriphMux);
    s_clockContext.armDiv = CLOCK_GetDiv(kCLOCK_ArmDiv);
    s_clockContext.is_valid = 1;

    /* When need to close PLL, we need to use bypass clock first and then power it down. */
    /* Power off SYS PLL */
    CCM_ANALOG->PLL_SYS_SET = CCM_ANALOG_PLL_SYS_BYPASS_MASK;
    CCM_ANALOG->PLL_SYS_SET = CCM_ANALOG_PLL_SYS_POWERDOWN_MASK;
    CCM_ANALOG->PFD_528_SET = CCM_ANALOG_PFD_528_PFD2_CLKGATE_MASK;
#if 0
    /* Power off USB1 PLL */
    CCM_ANALOG->PLL_USB1_SET = CCM_ANALOG_PLL_USB1_BYPASS_MASK;
    CCM_ANALOG->PLL_USB1_CLR = CCM_ANALOG_PLL_USB1_POWER_MASK;
    CCM_ANALOG->PFD_480_SET = CCM_ANALOG_PFD_480_PFD2_CLKGATE_MASK;
#endif
    /* Power off USB2 PLL */
    CCM_ANALOG->PLL_USB2_SET = CCM_ANALOG_PLL_USB2_BYPASS_MASK;
    CCM_ANALOG->PLL_USB2_CLR = CCM_ANALOG_PLL_USB2_POWER_MASK;
    /* Power off AUDIO PLL */
    CCM_ANALOG->PLL_AUDIO_SET = CCM_ANALOG_PLL_AUDIO_BYPASS_MASK;
    CCM_ANALOG->PLL_AUDIO_SET = CCM_ANALOG_PLL_AUDIO_POWERDOWN_MASK;
    /* Power off VIDEO PLL */
    CCM_ANALOG->PLL_VIDEO_SET = CCM_ANALOG_PLL_VIDEO_BYPASS_MASK;
    CCM_ANALOG->PLL_VIDEO_SET = CCM_ANALOG_PLL_VIDEO_POWERDOWN_MASK;
#if 0
    /* Power off ENET PLL */
    CCM_ANALOG->PLL_ENET_SET = CCM_ANALOG_PLL_ENET_BYPASS_MASK;
    CCM_ANALOG->PLL_ENET_SET = CCM_ANALOG_PLL_ENET_POWERDOWN_MASK;
#endif

    if ((LPM_PowerModeSysIdle == power_mode) || (LPM_PowerModeLowSpeedRun == power_mode))
    {
        /*ARM PLL as clksource*/
        /* 24 * 88 / 2 / 8 = 132MHz */
        CCM_ANALOG->PLL_ARM_CLR = CCM_ANALOG_PLL_ARM_POWERDOWN_MASK;
        CCM_ANALOG->PLL_ARM_SET = CCM_ANALOG_PLL_ARM_ENABLE_MASK |
                            CCM_ANALOG_PLL_ARM_BYPASS_MASK;
        CLOCK_SetDiv(kCLOCK_ArmDiv, 0x7);
        CCM_ANALOG->PLL_ARM_CLR = CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK;
        CCM_ANALOG->PLL_ARM_SET = CCM_ANALOG_PLL_ARM_DIV_SELECT(88);

        /* SET AHB, IPG, PER clock to 33MHz */
        CLOCK_SetMux(kCLOCK_PeriphMux, 0);
        CLOCK_SetDiv(kCLOCK_IpgDiv, 3);
        //CLOCK_SetDiv(kCLOCK_AhbDiv, 3);
        CLOCK_SetDiv(kCLOCK_AhbDiv, 0);

        CLOCK_SetMux(kCLOCK_PerclkMux, 0);
        CLOCK_SetDiv(kCLOCK_PerclkDiv, 0);

        /*Select ARM_PLL for pre_periph_clock */
        CLOCK_SetMux(kCLOCK_PrePeriphMux, 3);
        
        /*ARM PLL as clksource*/
        CCM_ANALOG->PLL_ARM_CLR = CCM_ANALOG_PLL_ARM_BYPASS_MASK;

        /* Wait CCM operation finishes */
        while (CCM->CDHIPR != 0)
        {
        }
    }
    else if ((LPM_PowerModeLPIdle == power_mode) || (LPM_PowerModeLowPowerRun == power_mode))
    {
        /*ARM PLL as clksource*/
        CCM_ANALOG->PLL_ARM |= CCM_ANALOG_PLL_ARM_ENABLE_MASK |
                            CCM_ANALOG_PLL_ARM_BYPASS_MASK;

        /*Select ARM_PLL for pre_periph_clock */
        CLOCK_SetDiv(kCLOCK_ArmDiv, 0x0);
        CLOCK_SetMux(kCLOCK_PrePeriphMux, 0x3);
        CLOCK_SetMux(kCLOCK_PeriphMux, 0x0);
        CLOCK_SetDiv(kCLOCK_IpgDiv, 0x1);
        CLOCK_SetDiv(kCLOCK_AhbDiv, 0x0);

        /*Set PERCLK_CLK_ROOT to 12Mhz*/
        CLOCK_SetMux(kCLOCK_PerclkMux, 0x0);
        CLOCK_SetDiv(kCLOCK_PerclkDiv, 0x0);

        /* Power off ARM PLL */
        CCM_ANALOG->PLL_ARM_SET = CCM_ANALOG_PLL_ARM_POWERDOWN_MASK;

        /* Wait CCM operation finishes */
        while (CCM->CDHIPR != 0)
        {
        }
    }
    else
    {
        /* Direct return from RUN and Suspend */
        return;
    }
#if 0
    CLOCK_DeinitUsb1Pfd(kCLOCK_Pfd0);
    CLOCK_DeinitUsb1Pfd(kCLOCK_Pfd1);
    CLOCK_DeinitUsb1Pfd(kCLOCK_Pfd2);
    CLOCK_DeinitUsb1Pfd(kCLOCK_Pfd3);
#endif
    CLOCK_DeinitSysPfd(kCLOCK_Pfd0);
    CLOCK_DeinitSysPfd(kCLOCK_Pfd1);
    CLOCK_DeinitSysPfd(kCLOCK_Pfd2);
    CLOCK_DeinitSysPfd(kCLOCK_Pfd3);
}

void LPM_RestorePLLs(lpm_power_mode_t power_mode)
{
    if ((LPM_PowerModeSysIdle == power_mode) || (LPM_PowerModeLowSpeedRun == power_mode))
    {
        /* ARM PLL as clksource*/
        CCM_ANALOG->PLL_ARM_CLR = CCM_ANALOG_PLL_ARM_POWERDOWN_MASK;
        CCM_ANALOG->PLL_ARM_SET = CCM_ANALOG_PLL_ARM_ENABLE_MASK |
                            CCM_ANALOG_PLL_ARM_BYPASS_MASK;
    }
    else if ((LPM_PowerModeLPIdle == power_mode) || (LPM_PowerModeLowPowerRun == power_mode))
    {
        /* Power on ARM PLL and wait for locked */
        /* Restore to 600M */
        if ((s_clockContext.pllarm & CCM_ANALOG_PLL_ARM_BYPASS_MASK) == 0)
        {
            CCM_ANALOG->PLL_ARM_CLR = CCM_ANALOG_PLL_ARM_POWERDOWN_MASK;
            //CCM_ANALOG->PLL_ARM_SET = CCM_ANALOG_PLL_ARM_ENABLE_MASK |
            //                    CCM_ANALOG_PLL_ARM_BYPASS_MASK;
        }
    }
    else if (LPM_PowerModeOverRun == power_mode)
    {
        if (s_clockContext.is_valid)
        {
            CCM_ANALOG->PLL_ARM_CLR = CCM_ANALOG_PLL_ARM_POWERDOWN_MASK;
            CCM_ANALOG->PLL_ARM_SET = CCM_ANALOG_PLL_ARM_ENABLE_MASK |
                            CCM_ANALOG_PLL_ARM_BYPASS_MASK;
        }
        else
        {
            return;
        }
    }
    else
    {
        /* Direct return from RUN and DSM */
        return;
    }

    CLOCK_SetDiv(kCLOCK_ArmDiv, s_clockContext.armDiv);
    CCM_ANALOG->PLL_ARM_CLR = CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK;
    CCM_ANALOG->PLL_ARM_SET = CCM_ANALOG_PLL_ARM_DIV_SELECT(s_clockContext.pllarm_loopdiv);
    if ((s_clockContext.pllarm & CCM_ANALOG_PLL_ARM_BYPASS_MASK) == 0)
    {
        while ((CCM_ANALOG->PLL_ARM & CCM_ANALOG_PLL_ARM_LOCK_MASK) == 0)
        {
        }
    }

    /* Restore AHB and IPG div */
    CCM->CBCDR = (CCM->CBCDR & ~(CCM_CBCDR_AHB_PODF_MASK | CCM_CBCDR_IPG_PODF_MASK | CCM_CBCDR_PERIPH_CLK_SEL_MASK))
                    | CCM_CBCDR_AHB_PODF(s_clockContext.ahbDiv)
                    | CCM_CBCDR_IPG_PODF(s_clockContext.ipgDiv)
                    | CCM_CBCDR_PERIPH_CLK_SEL(s_clockContext.periphSel);
    /* Restore Periphral clock */
    CCM->CSCMR1 = (CCM->CSCMR1 & ~CCM_CSCMR1_PERCLK_PODF_MASK)
                    | CCM_CSCMR1_PERCLK_PODF(s_clockContext.perDiv)
                    | CCM_CSCMR1_PERCLK_CLK_SEL(s_clockContext.perSel);
    
    /* Switch clocks back */
    //CLOCK_SetMux(kCLOCK_PerclkMux, s_clockContext.perSel);
    //CLOCK_SetMux(kCLOCK_PeriphMux, s_clockContext.periphSel);
    //CLOCK_SetMux(kCLOCK_PrePeriphMux, s_clockContext.preperiphSel);
    CCM->CBCMR = (CCM->CBCMR & ~CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK)
                    | CCM_CBCMR_PRE_PERIPH_CLK_SEL(s_clockContext.preperiphSel);

    /* Wait CCM operation finishes */
    while (CCM->CDHIPR != 0)
    {
    }

    if ((s_clockContext.pllarm & CCM_ANALOG_PLL_ARM_BYPASS_MASK) == 0)
    {
        CCM_ANALOG->PLL_ARM_CLR = CCM_ANALOG_PLL_ARM_BYPASS_MASK;
    }

    /* When need to enable PLL, we need to use bypass clock first and then switch pll back. */
    /* Power on SYS PLL and wait for locked */
    CCM_ANALOG->PLL_SYS_SET = CCM_ANALOG_PLL_SYS_BYPASS_MASK;
    CCM_ANALOG->PLL_SYS_CLR = CCM_ANALOG_PLL_SYS_POWERDOWN_MASK;
    CCM_ANALOG->PLL_SYS = s_clockContext.pllSys;
    if ((CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_POWERDOWN_MASK) == 0)
    {
        while ((CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_LOCK_MASK) == 0)
        {
        }
    }
    CCM_ANALOG->PFD_528_CLR = CCM_ANALOG_PFD_528_PFD2_CLKGATE_MASK;

    /* Restore USB1 PLL */
    CCM_ANALOG->PLL_USB1_SET = CCM_ANALOG_PLL_USB1_BYPASS_MASK;
    CCM_ANALOG->PLL_USB1_SET = CCM_ANALOG_PLL_USB1_POWER_MASK;
    CCM_ANALOG->PLL_USB1 = s_clockContext.pllUsb1;
    if ((CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_POWER_MASK) != 0)
    {
        while ((CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_LOCK_MASK) == 0)
        {
        }
    }
    CCM_ANALOG->PFD_480_CLR = CCM_ANALOG_PFD_480_PFD2_CLKGATE_MASK;

    /* Restore USB2 PLL */
    CCM_ANALOG->PLL_USB2_SET = CCM_ANALOG_PLL_USB2_BYPASS_MASK;
    CCM_ANALOG->PLL_USB2_SET = CCM_ANALOG_PLL_USB2_POWER_MASK;
    CCM_ANALOG->PLL_USB2 = s_clockContext.pllUsb2;
    if ((CCM_ANALOG->PLL_USB2 & CCM_ANALOG_PLL_USB2_POWER_MASK) != 0)
    {
        while ((CCM_ANALOG->PLL_USB2 & CCM_ANALOG_PLL_USB2_LOCK_MASK) == 0)
        {
        }
    }

    /* Restore AUDIO PLL */
    CCM_ANALOG->PLL_AUDIO_SET = CCM_ANALOG_PLL_AUDIO_BYPASS_MASK;
    CCM_ANALOG->PLL_AUDIO_CLR = CCM_ANALOG_PLL_AUDIO_POWERDOWN_MASK;
    CCM_ANALOG->PLL_AUDIO = s_clockContext.pllAudio;
    if ((CCM_ANALOG->PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_POWERDOWN_MASK) == 0)
    {
        while ((CCM_ANALOG->PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_LOCK_MASK) == 0)
        {
        }
    }

    /* Restore VIDEO PLL */
    CCM_ANALOG->PLL_VIDEO_SET = CCM_ANALOG_PLL_VIDEO_BYPASS_MASK;
    CCM_ANALOG->PLL_VIDEO_CLR = CCM_ANALOG_PLL_VIDEO_POWERDOWN_MASK;
    CCM_ANALOG->PLL_VIDEO = s_clockContext.pllVideo;
    if ((CCM_ANALOG->PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_POWERDOWN_MASK) == 0)
    {
        while ((CCM_ANALOG->PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_LOCK_MASK) == 0)
        {
        }
    }

    /* Restore ENET PLL */
    CCM_ANALOG->PLL_ENET_SET = CCM_ANALOG_PLL_ENET_BYPASS_MASK;
    CCM_ANALOG->PLL_ENET_SET = CCM_ANALOG_PLL_ENET_POWERDOWN_MASK;
    CCM_ANALOG->PLL_ENET = s_clockContext.pllEnet;
    if ((CCM_ANALOG->PLL_ENET & CCM_ANALOG_PLL_ENET_POWERDOWN_MASK) == 0)
    {
        while ((CCM_ANALOG->PLL_ENET & CCM_ANALOG_PLL_ENET_LOCK_MASK) == 0)
        {
        }
    }

    /* Restore SYS PLL PFD */
    CCM_ANALOG->PFD_528 = s_clockContext.pfd528;
    /* Restore USB1 PLL PFD */
    CCM_ANALOG->PFD_480 = s_clockContext.pfd480;
    s_clockContext.is_valid = 0;
}

#ifdef FSL_RTOS_FREE_RTOS
#if (configUSE_TICKLESS_IDLE == 1)

void LPM_InitTicklessTimer(void)
{
    gpt_config_t gptConfig;

    /* Init GPT for wakeup as FreeRTOS tell us */
    GPT_GetDefaultConfig(&gptConfig);
    gptConfig.clockSource = kGPT_ClockSource_LowFreq; /* 32K RTC OSC */
    //gptConfig.enableMode = false;                     /* Keep counter when stop */
    gptConfig.enableMode = true;                     /* Don't keep counter when stop */
    gptConfig.enableRunInDoze = true;
    /* Initialize GPT module */
    GPT_Init(vPortGetGptBase(), &gptConfig);
    GPT_SetClockDivider(vPortGetGptBase(), 1);

    /* Enable GPT Output Compare1 interrupt */
    GPT_EnableInterrupts(vPortGetGptBase(), kGPT_OutputCompare1InterruptEnable);
    NVIC_SetPriority(vPortGetGptIrqn(), configMAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    EnableIRQ(vPortGetGptIrqn());
}

#endif
#endif

bool LPM_Init(lpm_power_mode_t run_mode)
{
    uint32_t i;
    uint32_t tmp_reg = 0;

#ifdef FSL_RTOS_FREE_RTOS
#if (configUSE_TICKLESS_IDLE == 1)
    LPM_InitTicklessTimer();
#endif
#endif

    if (run_mode > LPM_PowerModeRunEnd)
    {
        return false;
    }
    CLOCK_SetMode(kCLOCK_ModeRun);

    CCM->CGPR |= CCM_CGPR_INT_MEM_CLK_LPM_MASK;

    /* Enable RC OSC. It needs at least 4ms to be stable, so self tuning need to be enabled. */
    XTALOSC24M->LOWPWR_CTRL |= XTALOSC24M_LOWPWR_CTRL_RC_OSC_EN_MASK;
    /* Configure RC OSC */
    XTALOSC24M->OSC_CONFIG0 = XTALOSC24M_OSC_CONFIG0_RC_OSC_PROG_CUR(0x4) | XTALOSC24M_OSC_CONFIG0_SET_HYST_MINUS(0x2) |
                              XTALOSC24M_OSC_CONFIG0_RC_OSC_PROG(0xA7) |
                              XTALOSC24M_OSC_CONFIG0_START_MASK | XTALOSC24M_OSC_CONFIG0_ENABLE_MASK;
    XTALOSC24M->OSC_CONFIG1 = XTALOSC24M_OSC_CONFIG1_COUNT_RC_CUR(0x40) | XTALOSC24M_OSC_CONFIG1_COUNT_RC_TRG(0x2DC);
    /* Wait at least 4ms */
    for (i = 0; i < 1000 * 1000; i++)
    {
        __NOP();
    }
    /* Add some hysteresis */
    tmp_reg = XTALOSC24M->OSC_CONFIG0;
    tmp_reg &= ~(XTALOSC24M_OSC_CONFIG0_HYST_PLUS_MASK | XTALOSC24M_OSC_CONFIG0_HYST_MINUS_MASK);
    tmp_reg |= XTALOSC24M_OSC_CONFIG0_HYST_PLUS(3) | XTALOSC24M_OSC_CONFIG0_HYST_MINUS(3);
    XTALOSC24M->OSC_CONFIG0 = tmp_reg;
    /* Set COUNT_1M_TRG */
    tmp_reg = XTALOSC24M->OSC_CONFIG2;
    tmp_reg &= ~XTALOSC24M_OSC_CONFIG2_COUNT_1M_TRG_MASK;
    tmp_reg |= XTALOSC24M_OSC_CONFIG2_COUNT_1M_TRG(0x2d7);
    XTALOSC24M->OSC_CONFIG2 = tmp_reg;
    /* Hardware requires to read OSC_CONFIG0 or OSC_CONFIG1 to make OSC_CONFIG2 write work */
    tmp_reg = XTALOSC24M->OSC_CONFIG1;
    XTALOSC24M->OSC_CONFIG1 = tmp_reg;

    /* ERR007265 */
    //IOMUXC_GPR->GPR1 |= IOMUXC_GPR_GPR1_GINT_MASK;

    /* Set DCDC DCM mode */
    //LPM_DCDC_Set_DCM_Mode();

    /* Initialize GPC to mask all IRQs */
    for (i = 0; i < LPM_GPC_IMR_NUM; i++)
    {
        GPC->IMR[i] = 0xFFFFFFFFU;
    }

    return true;
}

void LPM_Deinit(void)
{
    /* ERR007265 */
    IOMUXC_GPR->GPR1 &= ~IOMUXC_GPR_GPR1_GINT_MASK;
}
