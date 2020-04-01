/*!****************************************************************************
* \file OSAL_Api.c
* \author Rhod Davies
*
* \brief OSAL external interface implementation for FreeRTOS/Realtek SDK environment.
*
* Provides a wrapper around the FreeRTOS thread, message queue, and timer interfaces.
*
* Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#define __LINUX_ERRNO_EXTENSIONS__
#include <errno.h>
#include "OSAL_Api.h"
#include "FreeRTOS.h"
#include "timers.h"

#include "board.h"
#include "ksdk_mbedtls.h"

#include "fsl_debug_console.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_gpio.h"
#include "lwip/opt.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "netif/ethernet.h"
//#include "ethernetif.h"

//#include "aws_iot_config.h"
//#include "aws_iot_log.h"
//#include "aws_iot_version.h"
//#include "aws_iot_mqtt_client_interface.h"

#include "tinyprintf.h" //nishi

//#include "watchdog.h"

#define PRIORITIE_OFFSET 4 //nishi
//#include "timer_interface.h"
#include "OSAL_Api.h"
#include "LOG_Api.h"
#include "EFS_FileSystem.h"
#include "STO_FileSystem.h"


/*
 * Keeps seconds from Epoch
 */
static time_t OSAL_seconds;

bool test_OK = true;

/**
 * \brief Check_all_is_OK
 *
 * \return OK
 */
bool Check_all_is_OK(void)
{
    return test_OK;
}


/* stdout for tinyprintf library */
static void stdout_putf(void *unused, char c)
{
    (void)(unused);
    PUTCHAR(c);
}

void OSAL_printf(char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    tfp_format(NULL, stdout_putf, fmt, va);
    va_end(va);
}

int OSAL_sprintf(char *str, const char *format, ...)
{
    va_list ap;
    int retval;

    va_start(ap, format);
    retval = tfp_vsprintf(str, format, ap);
    va_end(ap);
    return retval;
}

int OSAL_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int retval;

    va_start(ap, format);
    retval = tfp_vsnprintf(str, size, format, ap);
    va_end(ap);
    return retval;
}

char * OSAL_fgets(char * buffer, int size, void * f)
{
    if (size < 2 || buffer == NULL)
    {
        return NULL;
    }
    else if (f == stdin)
    {
        memset(buffer, 0, size);
    
        // fgets is a blocking function so block until new line
        int bufferIndex = 0;
        char c;
        while (1)
        {
            c = GETCHAR();
            PRINTF("%c", c);    // Echo
            if (c == '\r')
            {
                PRINTF("\r\n"); // Echo
                break;
            }
            buffer[bufferIndex++] = c;
        }
        buffer[bufferIndex++] = '\0';

        return buffer;
    }
    else
    {
        return NULL;
    }  
}

void OSAL_sleep_ms(uint32_t milliseconds)
{
    int ticks = milliseconds / portTICK_PERIOD_MS;
    ////////LOG_Trace("vTaskDelay(%d)", ticks);
    vTaskDelay(ticks);
}

void * OSAL_GetTimerParam(void * p)
{
    if (p == NULL)
    {
        return NULL;
    }
    return pvTimerGetTimerID(p);
}


time_t OSAL_time()
{
    uint32_t  now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return OSAL_seconds + now / 1000;
}

void OSAL_SetTime(time_t newSystemTimeInSec)
{
    OSAL_seconds = newSystemTimeInSec;
}


uint32_t OSAL_GetTimeInSecondsSinceEpoch()
{
    return OSAL_time();
}

uint32_t OSAL_time_ms()
{
    static uint32_t start = 0;
    uint32_t  now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (start == 0)
    {
        start = now;
    }
    return now - start;
}

Timer_t OSAL_NewTimer(void (*callback)(void *), uint32_t milliseconds, bool repeat, Handle_t handle)
{
    Timer_t timer = NULL;
    if (callback == NULL)
    {
        return timer;
    }
    char name[] = "Timer";
    TickType_t ticks;
    // prevent overflow in pdMS_TO_TICKS macro for very large delays
    if (milliseconds > 4000000)
    {
        ticks = ( ( ( TickType_t ) ( milliseconds ) / ( TickType_t ) 1000 ) * ( TickType_t ) configTICK_RATE_HZ );
    }
    else
    {
        ticks = pdMS_TO_TICKS(milliseconds);
    }
    const int autoreload = repeat ? pdTRUE : pdFALSE;
    TimerHandle_t timerHandle = (void *)xTimerCreate(name, ticks, autoreload, handle, callback);
    //LOG_Trace("xTimerCreate(%s, %d, %s, %p, %p):%p", name, ticks, repeat ? "auto" : "single", handle, callback, timerHandle);
    int rc = xTimerStart(timerHandle, 0);
    //LOG_Trace("xTimerStart(%p, 0):%s", timerHandle, rc == pdPASS ? "PASS" : rc == pdFAIL ? "FAIL" : "Invalid");
    return timerHandle;
}

int OSAL_DestroyTimer(Timer_t timer)
{
    //LOG_Trace("(%p)", timer);
    return xTimerDelete(timer, portMAX_DELAY) == pdPASS ? 0 : -1;
}

typedef struct
{
    QueueHandle_t messageQueue;
    unsigned int maximumNumberOfMessages;
    unsigned int maximumMessageSize;
    bool block;
} MessageQueueDesc_t;

typedef struct
{
    MessagePriority_e priority;
    unsigned int size;
    void * buffer;
} MessageDesc_t;

MessageQueue_t OSAL_NewMessageQueue(const char * name, unsigned int maximumNumberOfMessages, unsigned int maximumMessageSize)
{
    //LOG_Trace("(\"%s\", %d, %d)", name, maximumNumberOfMessages, maximumMessageSize);
    MessageQueue_t messageQueue = NULL;
    MessageQueueDesc_t * messageQueueDescriptor = pvPortMalloc(sizeof *messageQueueDescriptor);
    if (messageQueueDescriptor == NULL)
    {
        LOG_Error("(pvPortMalloc(%d) failed", sizeof *messageQueueDescriptor);
        errno = ENOMEM;
        return messageQueue;
    }
    messageQueueDescriptor->messageQueue = xQueueCreate(maximumNumberOfMessages, sizeof(MessageDesc_t));
    if (messageQueueDescriptor->messageQueue == NULL)
    {
        LOG_Error("(xQueueCreate(%d, %d) failed", maximumNumberOfMessages, sizeof(MessageDesc_t));
        vPortFree(messageQueueDescriptor);
        errno = ENOMEM;
        return messageQueue;
    }
    messageQueueDescriptor->maximumNumberOfMessages = maximumNumberOfMessages;
    messageQueueDescriptor->maximumMessageSize = maximumMessageSize;
    messageQueueDescriptor->block = true;
    vQueueAddToRegistry(messageQueueDescriptor->messageQueue, name);
    return messageQueueDescriptor;
}

int OSAL_IsMessageQueueNonBlocking(MessageQueue_t messageQueue)
{
    MessageQueueDesc_t * messageQueueDescriptor = messageQueue;
    int ret = messageQueueDescriptor == NULL ? -1 : messageQueueDescriptor->block;
    //LOG_Trace("(%p):%d", messageQueue, ret);
    return ret;
}

int OSAL_SetMessageQueueNonBlock(MessageQueue_t messageQueue, bool nonBlock)
{
    MessageQueueDesc_t * messageQueueDescriptor = messageQueue;
    //LOG_Trace("(%p, %d)", messageQueue, nonBlock);
    if (messageQueueDescriptor == NULL)
    {
        errno = EBADR;
        return -1;
    }
    messageQueueDescriptor->block = !nonBlock;
    return 0;
}

int OSAL_GetMessageQueueSize(MessageQueue_t messageQueue)
{
    MessageQueueDesc_t * messageQueueDescriptor = messageQueue;
    int ret = -1;
    if (messageQueueDescriptor == NULL)
    {
        errno = EBADR;
    }
    else
    {
        ret = messageQueueDescriptor->maximumNumberOfMessages;
    }
    //LOG_Trace("(%p):%d", messageQueue, ret);
    return ret;
}

int OSAL_GetMessageQueueNumCurrentMessages(MessageQueue_t messageQueue)
{
    MessageQueueDesc_t * messageQueueDescriptor = messageQueue;
    int ret = -1;
    if (messageQueueDescriptor == NULL)
    {
        errno = EBADR;
    }
    else
    {
        ret = uxQueueMessagesWaiting(messageQueueDescriptor->messageQueue);
    }
    //LOG_Trace("(%p):%d", messageQueue, ret);
    return ret;
}

int OSAL_GetMessageQueueMaxMessageSize(MessageQueue_t messageQueue)
{
    MessageQueueDesc_t * messageQueueDescriptor = messageQueue;
    int ret = -1;
    if (messageQueueDescriptor == NULL)
    {
        errno = EBADR;
    }
    else
    {
        ret = messageQueueDescriptor->maximumMessageSize;
    }
    //LOG_Trace("(%p):%d", messageQueue, ret);
    return ret;
}

//////// [RE:workaround] Function missing return. Commented out as not needed.
/*
int OSAL_DestroyMessageQueue(MessageQueue_t messageQueue, const char * name)
{
    MessageQueueDesc_t * messageQueueDescriptor = messageQueue;
    LOG_Trace("(%p, \"%s\")", messageQueue, name ? name : "");
    if (messageQueueDescriptor == NULL)
    {
        LOG_Trace("null messageDescriptor");
        errno = EBADR;
        return -1;
    }
    if (name == NULL)
    {
        LOG_Trace("null name");
        errno = EBADR;
        return -1;
    }
    QueueHandle_t  queueHandle = messageQueueDescriptor->messageQueue;
    if (queueHandle == NULL)
    {
        LOG_Trace("null queueHandle");
        errno = EBADR;
        return -1;
    }
    vQueueUnregisterQueue(queueHandle);
    vQueueDelete(queueHandle);
    vPortFree(messageQueueDescriptor);
}
*/

int OSAL_SendMessage(MessageQueue_t messageQueue, const void * buffer, size_t size, MessagePriority_e priority)
{
    MessageQueueDesc_t * messageQueueDescriptor = messageQueue;
    //LOG_Trace("(%p, %p, %d, %d)", messageQueueDescriptor, buffer, size, priority);
    if (messageQueueDescriptor == NULL)
    {
        LOG_Error("null messageDescriptor");
        errno = EBADR;
        return -1;
    }
    else if (buffer == NULL)
    {
        LOG_Error("null buffer");
        errno = EBADR;
        return -1;
    }
    int messageSize = size > messageQueueDescriptor->maximumMessageSize ? messageQueueDescriptor->maximumMessageSize : size;
    MessageDesc_t md =
    {
        .priority = priority,
        .size = messageSize,
        .buffer = pvPortMalloc(messageSize),
    };
    if (md.buffer == NULL)
    {
        LOG_Error("malloc failed");
        errno = ENOMEM;
        return -1;
    }

    memcpy(md.buffer, buffer, messageSize);
    QueueHandle_t queueHandle = messageQueueDescriptor->messageQueue;
    int timeoutTicks = messageQueueDescriptor->block ? portMAX_DELAY : 0;
    BaseType_t rc = priority == MessagePriority_high ?
                    xQueueSendToFront(queueHandle, &md, timeoutTicks) :
                    xQueueSendToBack(queueHandle, &md, timeoutTicks);
    //LOG_Trace("(%p, %p, %d, %d):%d", queueHandle, buffer, size, priority, rc);
#if ENHANCED_DEBUG
    LOG_Warning("Queue count = %d",uxQueueMessagesWaiting(messageQueueDescriptor->messageQueue));
#endif
    return rc == pdTRUE ? messageSize : messageQueueDescriptor->block ? 0 : -1;
}

int OSAL_ReceiveMessage(MessageQueue_t messageQueue, void * buffer, size_t size, MessagePriority_e * priority)
{
    MessageQueueDesc_t * messageQueueDescriptor = messageQueue;
    if (messageQueueDescriptor == NULL)
    {
        LOG_Error("null messageDescriptor");
        errno = EBADR;
        return -1;
    }
    else if (buffer == NULL)
    {
        LOG_Error("null buffer");
        errno = EBADR;
        return -1;
    }
    else if (priority == NULL)
    {
        LOG_Error("null priority");
        errno = EBADR;
        return -1;
    }
    MessageDesc_t md = { 0 };
    QueueHandle_t queueHandle = messageQueueDescriptor->messageQueue;
    //LOG_Trace("(%p, %p, %d, %p)", queueHandle, buffer, size, priority);
    int timeoutTicks = messageQueueDescriptor->block ? portMAX_DELAY : 0;
    BaseType_t rc = xQueueReceive(queueHandle, &md, timeoutTicks);
    if (rc != pdTRUE && messageQueueDescriptor->block)
    {
        LOG_Error("xQueueReceive(%p, %p, %d) failed", queueHandle, &md, timeoutTicks);
    }
    *priority = md.priority;
    int messageSize = size > messageQueueDescriptor->maximumMessageSize ? messageQueueDescriptor->maximumMessageSize : size;
    messageSize = size > md.size ? md.size : size;
    memcpy(buffer, md.buffer, messageSize);
    vPortFree(md.buffer);
    //LOG_Trace("(%p, %p, %d, %p):%d", queueHandle, buffer, size, priority, rc);
    return rc == pdTRUE ? messageSize : messageQueueDescriptor->block ? 0 : -1;
}

Thread_t OSAL_NewThread(void (*function) (Handle_t), Handle_t arg)
{
    LOG_Trace("(%p, %p)", function, arg);
    Thread_t threadHandle = NULL;
    if (function == NULL)
    {
        errno = EBADR;
        return threadHandle;
    }
    BaseType_t pri = 0;
    TaskHandle_t createdTask;
    ////////BaseType_t retVal = xTaskCreate(function, "thread", 14 * 1024, arg, 0 + 3 + 4, &createdTask);   //////// [RE:mem]
    BaseType_t retVal = xTaskCreate(function, "thread", configMINIMAL_STACK_SIZE, arg, tskIDLE_PRIORITY + 0 + PRIORITIE_OFFSET, &createdTask);

    if (pdPASS == retVal)
    {
        threadHandle = createdTask;
    }

    return threadHandle;
}

void OSAL_KillThread(Thread_t threadHandle)
{
    LOG_Trace("(%p)", threadHandle);

    if (threadHandle == NULL)
    {
        errno = EBADR;
        LOG_Error("Invalid Thread Handler %p", threadHandle);
        return;
    }

    TaskHandle_t thread = (TaskHandle_t)threadHandle;

    vTaskSuspend(thread);
    vTaskDelete(thread);
}

int OSAL_InitMutex(Mutex_t * mutex, const MutexAttr_t * attr)
{
    //LOG_Trace("(%p, %p)", mutex, attr);
    if (mutex == NULL)
    {
        errno = EBADR;
        return -1;
    }
    *mutex = xSemaphoreCreateRecursiveMutex();
    return *mutex ? 0 : -1;
}

int OSAL_LockMutex(Mutex_t * mutex)
{
    //LOG_Trace("%p", mutex);
    if (mutex == NULL)
    {
        errno = EBADR;
        return -1;
    }
    return xSemaphoreTakeRecursive(*mutex, portMAX_DELAY);
}

int OSAL_TryLockMutex(Mutex_t * mutex)
{
    //LOG_Trace("%p", mutex);
    if (mutex == NULL)
    {
        errno = EBADR;
        return -1;
    }
    return xSemaphoreTakeRecursive(*mutex, 0) == pdTRUE ? 0 : MUTEX_BUSY;
}

int OSAL_UnLockMutex(Mutex_t * mutex)
{
    //LOG_Trace("%p", mutex);
    if (mutex == NULL)
    {
        errno = EBADR;
        return -1;
    }
    return xSemaphoreGiveRecursive(*mutex);
}

int OSAL_DestroyMutex(Mutex_t * mutex)
{
    //LOG_Trace("%p", mutex);
    if (mutex == NULL)
    {
        errno = EBADR;
        return -1;
    }
    vSemaphoreDelete(*mutex);
    return 0;
}

SemaphoreHandle_t log_mutex;
#define MAX_LOG_ENTRY_SIZE   512
extern QueueHandle_t debugQueue;

void OSAL_Log(const char *format, ...)
{
    uint8_t *buffer;
    if (NULL == log_mutex)
    {
        if ((log_mutex = xSemaphoreCreateMutex()) == NULL)
        {
            /* We have a LOG issue, how to log a LOG issue? */
            return;
        }
    }

    if (debugQueue)
    {
        buffer = pvPortMalloc(MAX_LOG_ENTRY_SIZE);
        if (buffer)
        {
            va_list varArgList;
            va_start(varArgList, format);

            tfp_vsnprintf(buffer,MAX_LOG_ENTRY_SIZE,format,varArgList);
            buffer[MAX_LOG_ENTRY_SIZE-1] = '\0';

            va_end(varArgList);

            if(xQueueSendToBack(debugQueue, &buffer, 100) != pdPASS )
            {
                xSemaphoreTake(log_mutex, portMAX_DELAY);
                OSAL_printf(LOG_RED "Failed to send log message !!!\r\n");
                vPortFree(buffer);
                xSemaphoreGive(log_mutex);
            }
        }
        else
        {
            OSAL_printf("Buffer NOT Allocated !!\r\n");
        }
    }
    else
    {
        xSemaphoreTake(log_mutex, portMAX_DELAY);
        va_list varArgList;
        va_start(varArgList, format);

        tfp_format(NULL, stdout_putf, format, varArgList);

        va_end(varArgList);
        PUTCHAR('\r');
        xSemaphoreGive(log_mutex);
    }
}

void * calloc(size_t nmemb, size_t size)
{
    void * p = pvPortMalloc(nmemb * size);
    LOG_Malloc("(nmemb %d, size %d):%p heap:%d", nmemb, size, p, xPortGetFreeHeapSize());
    if (p == NULL)
    {
        errno = ENOMEM;
    }
    else
    {
        memset(p, 0, nmemb * size);
    }
    return p;
}

////////// [RE:workaround] Disabled realloc
/********
void * realloc(void * old, size_t size)
{
    void * p = pvPortReAlloc(old, size);
    LOG_Malloc("(size %d):%p heap:%d", size, p, xPortGetFreeHeapSize());
    if (p == NULL)
    {
        errno = ENOMEM;
    }
    return p;
}
********/
/********
void * realloc(void * old, size_t size)
{
    errno = ENOSYS;
    return NULL;
}
********/

void * malloc(size_t size)
{
    void * p = pvPortMalloc(size);
    LOG_Malloc("(size %d):%p heap:%d", size, p, xPortGetFreeHeapSize());
    if (p == NULL)
    {
        errno = ENOMEM;
    }
    return p;
}

void free(void * p)
{
    vPortFree(p);
    LOG_Malloc("(%p) heap:%d", p, xPortGetFreeHeapSize());
}

char * strdup(const char * s) // needed to force use of our malloc
{
    char * new = malloc(strlen(s) + 1);
    if (new)
    {
        strcpy(new, s);
    }
    else
    {
        errno = ENOMEM;
    }
    return new;
}
typedef struct MemoryDescriptor_tag
{
    size_t blockSize;
    size_t amountStored;
} MemoryDescriptor_t;

MemoryHandle_t OSAL_MemoryRequest(MemoryPoolHandle_t pool, size_t size)
{
    MemoryHandle_t handle;
    handle = malloc(size + sizeof(MemoryDescriptor_t));
    if ( handle )
    {
        ((MemoryDescriptor_t *) handle)->blockSize = size;
        ((MemoryDescriptor_t *) handle)->amountStored = size;
        handle =  (((MemoryDescriptor_t *) handle) + 1) ;
    }

    return handle;
}

void OSAL_Free(MemoryHandle_t handle)
{
    if (NULL != handle)
    {
        (((MemoryDescriptor_t *) handle) -1)->blockSize = 0;
        (((MemoryDescriptor_t *) handle) -1)->amountStored = 0;
        free ((MemoryDescriptor_t*)handle - 1);
    }
}

size_t OSAL_GetBlockSize( MemoryHandle_t handle )
{
    size_t size = 0;
    if ( NULL != handle )
    {
        size = (((MemoryDescriptor_t *) handle) -1)->blockSize ;
    }
    return size;
}

size_t OSAL_GetAmountStored( MemoryHandle_t handle )
{
    size_t size = 0;
    if ( NULL != handle )
    {
        size = (((MemoryDescriptor_t *) handle) -1)->amountStored ;
    }
    return size;
}

size_t OSAL_SetAmountStored( MemoryHandle_t handle, size_t newAmount )
{
    size_t size = 0;
    if ( NULL != handle )
    {
        if ( newAmount <= (((MemoryDescriptor_t *) handle) -1)->blockSize )
        {
            size = (((MemoryDescriptor_t *) handle) -1)->amountStored = newAmount ;
        }
    }
    return size;
}

void* OSAL_GetMemoryPointer(MemoryHandle_t handle)
{
    return handle;
}

int OSAL_watchdog_init(uint32_t timeout_ms)
{
    LOG_Trace("(%d)", timeout_ms);
    watchdog_init(timeout_ms);
    watchdog_start();
    return 0;
}

void OSAL_watchdog_stop(void)
{
    LOG_Trace("");
    watchdog_stop();
}

void OSAL_watchdog_refresh(void)
{
    LOG_Trace("");
    if(Check_all_is_OK())
    {
        watchdog_refresh();
    }
}

void OSAL_watchdog_suspend_refresh(void)
{
    test_OK = false;
}

Handle_t OSAL_StoreOpen(const char * storeName, OSAL_AccessMode_e accessMode)
{
    return STO_Open(storeName, accessMode);
    //////// [RE:workaround] WisafeGateway app does not play well with concurrent access to EFS of Store & Key APIs
    //return NULL;
}

int OSAL_StoreWrite(Handle_t handle, const void * buffer, size_t nBytes)
{
    return STO_Write(handle, buffer, nBytes);
}

int OSAL_StoreRead(Handle_t handle, void* buffer, size_t nBytes)
{
    return STO_Read(handle, buffer, nBytes);
}

int OSAL_StoreClose(Handle_t handle)
{
    return STO_Close(handle);
}

int OSAL_StoreAtomicRename(const char * oldName, const char * newName)
{
    return STO_Rename(oldName, newName);
}

int OSAL_StoreRemove(const char * storeName)
{
    return STO_Remove(storeName);
}

int OSAL_StoreSize(const char* storeName)
{
    return STO_FileSize(storeName);
    //////// [RE:workaround] WisafeGateway app does not play well with concurrent access to EFS of Store & Key APIs
    //return 0;
}

bool OSAL_StoreExist(const char* storeName)
{
    return STO_FileExist(storeName);
    //////// [RE:workaround] WisafeGateway app does not play well with concurrent access to EFS of Store & Key APIs
    //return false;
}

//////// [RE:workaround] Function missing return. Commented out as not needed.
/*
int setvbuf(FILE *__restrict a, char *__restrict b, int c, size_t d)
{
    LOG_Warning("setvbuf");
}
*/

//////// [RE:platform] Can't use getenv
/*********
char * getenvDefault(char * name, char * defaultValue)
{
    char * value = getenv(name);
    return value ? value : defaultValue;
}
*********/
char * getenvDefault(char * name, char * defaultValue)
{
    LOG_Trace("getenvDefault returning default value: %s", defaultValue);
    return defaultValue;
}

int OSAL_InitBinarySemaphore(Semaphore_t * semaphore)
{
    //LOG_Trace("(%p", semaphore);
    if (semaphore == NULL)
    {
        errno = EBADR;
        return -1;
    }
    *semaphore = xSemaphoreCreateBinary();
    return *semaphore ? 0 : -1;
}

int OSAL_GiveBinarySemaphore(Semaphore_t *semaphore)
{
    //LOG_Trace("%p", semaphore);
    if (semaphore == NULL)
    {
        errno = EBADR;
        return -1;
    }
    return xSemaphoreGive(*semaphore);
}

int OSAL_TakeBinarySemaphore(Semaphore_t *semaphore)
{
    //LOG_Trace("%p", semaphore);
    if (semaphore == NULL)
    {
        errno = EBADR;
        return -1;
    }
    return xSemaphoreTake(*semaphore, portMAX_DELAY );
}
