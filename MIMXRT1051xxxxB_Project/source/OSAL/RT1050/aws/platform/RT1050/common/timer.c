/**
 * @file timer.c
 * @brief OSAL implementation of the timer interface.
 */

#include <OSAL_Api.h>
#include <stdint.h>
#include <stdbool.h>
//nishi
#include "timer_platform.h"
#if 0
bool has_timer_expired(Timer *timer)
{
#if 0
	uint32_t now = OSAL_time_ms();
	return timer->end_time <= now;
#endif
	return false;
}

void countdown_ms(Timer *timer, uint32_t timeout)
{
#if 0
	uint32_t now = OSAL_time_ms();
	timer->end_time = now + timeout;
#endif
	return false;
}

uint32_t left_ms(Timer *timer)
{
#if 0
	uint32_t now = OSAL_time_ms();
	return now <= timer->end_time ? 0 : timer->end_time - now;
#endif
	return 0;
}

void countdown_sec(Timer *timer, uint32_t timeoutInSec)
{
#if 0
    countdown_ms(timer, timeoutInSec * 1000);
	//uint32_t now;
	//timer->end_time = OSAL_time_ms() + timeout * 1000;
#endif
}

void OSAL_init_timer(Timer *timer)
{
	timer->end_time = 0;
}

#endif
