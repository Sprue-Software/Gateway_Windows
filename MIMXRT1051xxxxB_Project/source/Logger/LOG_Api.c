/*!****************************************************************************
*
* \file LOG_Api.c
*
* \author Evelyne Donnaes
*
* \brief Logging API Implementation
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#define _POSIX_C_SOURCE 199309L ///< for clock_gettime() see man clock_gettime

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "LOG_Api.h"

LOG_Control_t LOG_Control =
{
    .err = true,  ///< LOG_Error enabled by default
    .war = true,  ///< LOG_Warning enabled by default
    .inf = true,  ///< LOG_Info enabled by default
    ////////.trc = false, ///< LOG_Trace disabled by default
    ////////.mem = false, ///< LOG_MAlloc disabled by default
    .trc = true, ///< LOG_Trace disabled by default
    .mem = false, ///< LOG_MAlloc disabled by default
};


/**
 * \brief   Initialise logging parameters from environment variables
 *
 */

void LOG_Init(void)
{
    struct
    {
        char name[4];
        bool * pval;
    } envp[] =
    {
        { .name = "ERR", .pval = &LOG_Control.err },
        { .name = "WAR", .pval = &LOG_Control.war },
        { .name = "INF", .pval = &LOG_Control.inf },
        { .name = "TRC", .pval = &LOG_Control.trc },
        { .name = "MEM", .pval = &LOG_Control.mem }
    };
    for (int i = 0; i < sizeof envp / sizeof envp[0]; i++)
    {
        char * val = getenvDefault(envp[i].name, NULL); //////// [RE:platform] Can't use getenv
        if (val != NULL)
        {
            *envp[i].pval = atoi(val);
        }
    }
}

/**
 * \brief   Enable/Disable LOG_Error output
 *
 * \param   enable - true enables LOG_Error output
 *
 * \return  bool - true if LOG_Error output was enabled
 *
 */
bool LOG_EnableError(bool enable)
{
    bool err = LOG_Control.err;
    LOG_Control.err = enable;
    return err;
}

/**
 * \brief   Enable/Disable LOG_Warning output
 *
 * \param   enable - true enables LOG_Warning output
 *
 * \return  bool - true if LOG_Warning output was enabled
 *
 */
bool LOG_EnableWarning(bool enable)
{
    bool war = LOG_Control.war;
    LOG_Control.war = enable;
    return war;
}

/**
 * \brief   Enable/Disable LOG_Info output
 *
 * \param   enable - true enables LOG_Info output
 *
 * \return  bool - true if LOG_Info output was enabled
 *
 */
bool LOG_EnableInfo(bool enable)
{
    bool inf = LOG_Control.inf;
    LOG_Control.inf = enable;
    return inf;
}

/**
 * \brief   Enable/Disable LOG_Trace output
 *
 * \param   enable - true enables LOG_Trace output
 *
 * \return  bool - true if LOG_Trace output was enabled
 *
 */
bool LOG_EnableTrace(bool enable)
{
    bool trc = LOG_Control.trc;
    LOG_Control.trc = enable;
    return trc;
}

/**
 * \brief   Enable/Disable LOG_Malloc output
 *
 * \param   enable - true enables LOG_Malloc output
 *
 * \return  bool - true if LOG_Malloc output was enabled
 *
 */
bool LOG_EnableMalloc(bool enable)
{
    bool mem = LOG_Control.mem;
    LOG_Control.mem = enable;
    return mem;
}

