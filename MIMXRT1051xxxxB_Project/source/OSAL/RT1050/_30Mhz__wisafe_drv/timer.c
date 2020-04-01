/*******************************************************************************
 * \file	timer.c
 * \brief 	Timer1 initialisation and setting to 0.5msec intervals. Handling
 * 			timer interrupts and keep track of time counts
 * \note 	Project: 	WG2. Low cost wireless gateway
 * \author  Abdul Ben-Rashed
 ******************************************************************************/

#define _TIMER_C_

#include <wisafe_main.h>
#include "fsl_debug_console.h"
#include "fsl_gpt.h"
#include "pin_mux.h"
#include "clock_config.h"
#include <timer.h>


/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/


static volatile bool gptIsrFlag = false;


/*******************************************************************************
 * Code
 ******************************************************************************/



/*******************************************************************************
 * \brief	Timer interrupt handler.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void EXAMPLE_GPT_IRQHandler(void)
{
    // Clear interrupt flag.
    GPT_ClearStatusFlags(EXAMPLE_GPT, kGPT_OutputCompare1Flag);

    gptIsrFlag = true;
    timercounts++;
    if((timercounts%250) == 0)
    {
    	SysTime++;					//Increment every 125msec
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}



/*******************************************************************************
 * \brief	Initialise Timer.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Init_Timer (void)
{
    uint32_t gptFreq;
    gpt_config_t gptConfig;


    /*Clock setting for GPT*/
    CLOCK_SetMux(kCLOCK_PerclkMux, EXAMPLE_GPT_CLOCK_SOURCE_SELECT);
    CLOCK_SetDiv(kCLOCK_PerclkDiv, EXAMPLE_GPT_CLOCK_DIVIDER_SELECT);

    GPT_GetDefaultConfig(&gptConfig);

    /* Initialize GPT module */
    GPT_Init(EXAMPLE_GPT, &gptConfig);

    /* Divide GPT clock source frequency by 3 inside GPT module */
    GPT_SetClockDivider(EXAMPLE_GPT, 3);

    /* Get GPT clock frequency */
    gptFreq = EXAMPLE_GPT_CLK_FREQ;

    /* GPT frequency is divided by 3 inside module */
    gptFreq /= 6000;

    /* Set both GPT modules to 1 second duration */
    GPT_SetOutputCompareValue(EXAMPLE_GPT, kGPT_OutputCompare_Channel1, gptFreq);

    /* Enable GPT Output Compare1 interrupt */
    GPT_EnableInterrupts(EXAMPLE_GPT, kGPT_OutputCompare1InterruptEnable);

    /* Enable at the Interrupt */
    EnableIRQ(GPT_IRQ_ID);

    /* Start Timer */
    //PRINTF("\r\nStarting GPT timer ...");
    GPT_StartTimer(EXAMPLE_GPT);
}



/*******************************************************************************
 * \brief       Get timer counts.
 *
 * \param       None.
 * \return      None.
 ******************************************************************************/

volatile uint32_t GetCurrentTimerCount(void)
{
    return timercounts;
}



/*******************************************************************************
 * \brief	Delay number of Milli-seconds.
 *
 * \param   msec. Number of msec to wait in this function.
 * \return	None.
 ******************************************************************************/

void Delay(uint32_t msec)
{
	uint32_t starttime;
	uint32_t intervalcounts;

	intervalcounts = msec<<1;//*2
	starttime = timercounts;

	while( (timercounts-starttime) < (intervalcounts) );
}
