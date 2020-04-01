/*!****************************************************************************
*
* \file LOG_Api.h
*
* \author Evelyne Donnaes
*
* \brief Logging API
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _LOG_API_H_
#define _LOG_API_H_

#include <stdbool.h>
#include "OSAL_Api.h"

typedef struct
{
    bool err;
    bool war;
    bool inf;
    bool trc;
    bool mem;
} LOG_Control_t;

extern LOG_Control_t LOG_Control;

#include <assert.h>

void LOG_Init(void);
bool LOG_EnableError(bool enable);
bool LOG_EnableWarning(bool enable);
bool LOG_EnableInfo(bool enable);
bool LOG_EnableTrace(bool enable);
bool LOG_EnableMalloc(bool enable);

#define LOG_(type, fmt, ...) do { uint32_t ms = OSAL_time_ms(); \
                                  OSAL_Log("%5d.%03d " type " %s: " fmt "\n", ms / 1000, ms % 1000, __func__ , ## __VA_ARGS__); \
                             } while (0);

#define LOG_NONE     "\033[00m"
#define LOG_RED      "\033[22;31m"
#define LOG_LIGHTRED "\033[01;31m"
#define LOG_GREEN    "\033[01;32m"
#define LOG_YELLOW   "\033[01;33m"
#define LOG_BLUE     "\033[01;34m"
#define LOG_MAGENTA  "\033[01;35m"
#define LOG_CYAN     "\033[01;36m"

// Warning logging
#define LOG_Warning(fmt, ...) /*if (LOG_Control.war)*/ if (true) LOG_("WAR", LOG_LIGHTRED fmt LOG_NONE, ## __VA_ARGS__)
#define LOG_WarningC(fmt, ...) /*if (LOG_Control.war)*/ if (true) LOG_("WAR", fmt LOG_NONE, ## __VA_ARGS__)

// Error logging
#define LOG_Error(fmt, ...) /*if (LOG_Control.err)*/ if (true) LOG_("ERR", LOG_RED fmt LOG_NONE, ## __VA_ARGS__)
#define LOG_ErrorC(fmt, ...) /*if (LOG_Control.err)*/ if (true) LOG_("ERR", fmt LOG_NONE, ## __VA_ARGS__)

// Info logging
#define LOG_Info(fmt, ...) /*if (LOG_Control.inf)*/ if (true) LOG_("INF", fmt LOG_NONE, ## __VA_ARGS__)
#define LOG_InfoC(fmt, ...) /*if (LOG_Control.inf)*/ if (true) LOG_("INF", fmt LOG_NONE, ## __VA_ARGS__)

// Trace logging
#define LOG_Trace(fmt, ...) /*if (LOG_Control.trc)*/ if (true) LOG_("TRC", fmt LOG_NONE, ## __VA_ARGS__)
#define LOG_TraceC(fmt, ...) /*if (LOG_Control.trc)*/ if (true) LOG_("TRC", fmt LOG_NONE, ## __VA_ARGS__)

// Malloc/free logging
#define LOG_Malloc(fmt, ...) /*if (LOG_Control.mem)*/ if (false) LOG_("MEM", fmt LOG_NONE, ## __VA_ARGS__)
#define LOG_MallocC(fmt, ...) /*if (LOG_Control.mem)*/ if (false) LOG_("MEM", fmt LOG_NONE, ## __VA_ARGS__)



#endif /* _LOG_API_H_ */
