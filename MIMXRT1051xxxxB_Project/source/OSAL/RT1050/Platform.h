/*!****************************************************************************
* \file Platform.h
* \author Rhod Davies
*
* \brief Implementation dependant definitions
*
* Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef Platform_h
#define Platform_h

#include "FreeRTOS.h"
#include "semphr.h"

typedef SemaphoreHandle_t Platform_Mutex_t;
typedef SemaphoreHandle_t Platform_Semaphore_t;
typedef void Platform_MutexAttr_t;

#define MUTEX_BUSY EBUSY

typedef void * Platform_Handle_t;       /**< Generic opaque handle */

extern int errno;
#endif
