/*!****************************************************************************
 * \file Watchdog.c
 * \author Rhod Davies
 *
 * \brief * Watchdog - watchdog thread handler
 * refreshes watchdog if Check_all_is_OK() returns true.
 *
 * Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
//#include <unistd.h>

#include "LOG_Api.h"
#include "OSAL_Api.h"
#include "OSAL_Debug.h"
extern int errno;
bool OK = true;

/**
 * \brief Check_all_is_OK
 *
 * \return OK
 */
bool Check_all_is_OK_test(void)
{
    return OK;
}

/**
 * \brief Watchdog timer handler
 * 
 * Checks sleeping for half of the watchdog period and refreshing the watchdog
 * if Check_all_is_OK_test returns true.
 *
 * \param param not used
 *
 * \return none
 *
 */
void Watchdog(void* param)
{
    (void)param;
    if (Check_all_is_OK_test())
    {
        LOG_Trace("WDT Refresh");
        OSAL_watchdog_refresh();
    } else {
        LOG_Warning("Not refreshing watchdog");
    }
}

/**
 * \brief Watchdog initialisation
 * 
 * initializes the watchdog, and starts the watchdog thread if successful.
 *
 * \param timeout - watchdog period in milliseconds.
 *
 */
void Watchdog_Init(int timeout)
{
    if (OSAL_watchdog_init(timeout) == -1)
    {
        LOG_Info("WDTdisabled");
    }
    else
    {
        /* Create a periodic timer to feed WDT if Check_all_is_OK() is true */
        Timer_t watchdogTimer = OSAL_NewTimer(Watchdog, timeout / 6, true, NULL);

        if (NULL == watchdogTimer)
        {
            LOG_Error("Could not create timer for WDT.");
        }
    }
}
