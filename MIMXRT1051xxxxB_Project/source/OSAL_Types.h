/*!****************************************************************************
* \file OSAL_Types.h
* \author Carl Pickering
*
* \brief Types used in the OSAL
*
* Types used in the OSAL Api
*
*
* Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#ifndef OSAL_TYPES_H
#define OSAL_TYPES_H

#include "../OSAL/RT1050/Platform.h"

typedef Platform_Handle_t Handle_t;
typedef Platform_Mutex_t Mutex_t;
typedef Platform_Semaphore_t Semaphore_t;
typedef Platform_MutexAttr_t MutexAttr_t;

typedef Handle_t Thread_t;
typedef Handle_t MessageQueue_t;
typedef Handle_t Timer_t;
typedef Handle_t MemoryHandle_t;
typedef Handle_t MemoryPoolHandle_t;

typedef enum
{
    MessagePriority_low    = 0,
    MessagePriority_medium = 1,
    MessagePriority_high   = 2
} MessagePriority_e;

#endif
