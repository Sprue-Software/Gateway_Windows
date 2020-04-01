/*
 * timer.h
 *
 *  Created on: 13 Apr 2018
 *      Author: abenrashed
 */

#ifndef _TIMER_H_
#define _TIMER_H_



#include <stdint.h>



#define GPT_IRQ_ID GPT2_IRQn
#define EXAMPLE_GPT GPT2
#define EXAMPLE_GPT_IRQHandler GPT2_IRQHandler

/* Select IPG Clock as PERCLK_CLK clock source */
#define EXAMPLE_GPT_CLOCK_SOURCE_SELECT (0U)
/* Clock divider for PERCLK_CLK clock source */
#define EXAMPLE_GPT_CLOCK_DIVIDER_SELECT (0U)
/* Get source clock for GPT driver (GPT prescaler = 0) */
#define EXAMPLE_GPT_CLK_FREQ (CLOCK_GetFreq(kCLOCK_IpgClk) / (EXAMPLE_GPT_CLOCK_DIVIDER_SELECT + 1U))

#define ONE_SECOND			2000
#define THREE_SECOND		6000
#define THREE_SECOND_PLUS	6500
#define FIVE_SECOND			10000
#define SIX_SECOND			12000
#define ONE_MILLISECOND		2
#define TWO_MILLISECOND		4
#define THREE_MILLISECOND  	6
#define FOUR_MILLISECOND	8
#define FIFTY_MILLISECOND	100

#ifdef _TIMER_C_
#undef _TIMER_C_

#include "board.h"


volatile uint32_t timercounts = 0;
volatile uint32_t SysTime = 0;

void Init_Timer(void);
void Delay(uint32_t msec);
volatile uint32_t GetCurrentTimerCount(void);

#else

extern volatile uint32_t timercounts;
extern volatile uint32_t SysTime;

extern void Init_Timer(void);
extern void Delay(uint32_t msec);
extern volatile uint32_t GetCurrentTimerCount(void);

#endif // _TIMER_C_

#endif // TIMER_H_
