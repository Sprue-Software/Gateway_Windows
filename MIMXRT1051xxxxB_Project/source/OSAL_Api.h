/*!****************************************************************************
* \file OSAL_Api.h
* \author Rhod Davies
*
* \brief OSAL external interface
*
* Provides a wrapper for posix thread, message queue, mutex, and timer interfaces.
*
*
* Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#ifndef OSAL_H
#define OSAL_H

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
//#include <fcntl.h>
#include "OSAL_Types.h"
//End of Line test
struct Status_Gate
{
    uint8_t Software_version[3];
    const char *Serial_Number;
    const char *MAC_address;
    uint8_t Battery_connected;
    int Battery_voltage[2];
    uint8_t Charging_Fault;
    uint8_t Switch_Position;
    uint8_t Tablet_Connected;
    uint8_t Newtork_Status;
    uint8_t ZWave_module;
    uint8_t *Board_Type;
};

extern struct Status_Gate Gateway_Status_request;
/**
 * \brief Platform specific printf function.
 *        Some platforms has some limitations and issues so platform specific
 *        printf function is implemented to handle them.
 *
 * \param   format - printf format string.
 * \param   ...    - varargs for printf
 *
 */
void OSAL_printf(char *fmt, ...);

/**
 * \brief Platform specific sprintf function.
 *
 * \param   str    - buffer to write formatted into
 * \param   format - printf format string.
 * \param   ...    - varargs for printf
 *
 * \return all formatted string length
 */
int OSAL_sprintf(char *str, const char *format, ...);

/**
 * \brief Platform specific sprintf function.
 *
 * \param   str    - buffer to write formatted into
 * \param   size   - maximum string size. (capacity of buffer)
 * \param   format - printf format string.
 * \param   ...    - varargs for printf
 *
 * \return all formatted string length
 */
int OSAL_snprintf(char *str, size_t size, const char *format, ...);

/**
 * \brief   read a string into a buffer
 *
 * reads in at most one less than size characters and stores them into
 * the buffer pointed to by s. Reading stops after a newline.
 * If a newline is read, it is stored into the buffer.
 * A terminating null byte ('\0') is stored after the last character
 * in the buffer.
 *
 * \param   buffer buffer to read into
 * \param   size size of buffer
 * \param   f FILE pointer - only stdin supported at present
 */
char * OSAL_fgets(char * buffer, int size, void * f);

/**
 * \brief   sleep in milliseconds
 *
 * \param   milliseconds number of milliseconds to sleep.
 */
void OSAL_sleep_ms(uint32_t milliseconds);

/**
 * \brief   get time in seconds since epoch
 *
 * \return  time in seconds
 */
uint32_t OSAL_GetTimeInSecondsSinceEpoch();

/**
 * \brief   get elapsed time in milliseconds
 *
 * \return  uint32_t - on first call 0
 *                   on subsequent calls time elapsed in ms on realtime clock since first call.
 */
uint32_t OSAL_time_ms();

/**
 * \brief   Call function(arg) in a new thread.
 *
 * \param   function function to be called as new thread.
 * \param   arg argument to pass to function.
 * \return  Thread_t handle to thread, NULL / 0 if error.
 */
Thread_t OSAL_NewThread(void (*function) (Handle_t), Handle_t arg);

/**
 * \brief   Kills an already created thread.
 *
 * \param   threadHandle handle of to be killed thread
 *
 * \return  none
 */
void OSAL_KillThread(Thread_t threadHandle);

/**
 * \brief   create a new message queue
 *
 * \param   name message queue name
 * \param   maximumNumberOfMessages maximum number of messages allowed in queue
 * \param   maximumMessageSize maximum message size
 * \return  MessageQueue_t handle to message queue, NULL / 0 if error.
 */
MessageQueue_t OSAL_NewMessageQueue(const char * name, unsigned int maximumNumberOfMessages, unsigned int maximumMessageSize);

/**
 * \brief   send a message on the specified queue
 *
 * \param   messageQueue message queue
 * \param   buffer pointer to buffer
 * \param   size number of bytes to send
 * \param   priority message priority Low, Medium, or High
 * \return  number of bytes sent, -1 if error.
*/
int OSAL_SendMessage(MessageQueue_t messageQueue, const void * buffer, size_t size, MessagePriority_e priority);

/**
 * \brief   receive a message from the specified queue
 *
 * \param   messageQueue message queue
 * \param   buffer pointer to buffer
 * \param   size maximum number of bytes to receive
 * \param   priority pointer to where message priority is to be stored.
 * \return  number of bytes received, -1 if error.
*/
int OSAL_ReceiveMessage(MessageQueue_t messageQueue, void * buffer, size_t size, MessagePriority_e * priority);

/**
 * \brief   get the Non-Blocking status of the queue
 *
 * \param   messageQueue message queue
 * \return  1 if blocking, -1 if error, 0 if success
*/
int OSAL_IsMessageQueueNonBlocking(MessageQueue_t messageQueue);

/**
 * \brief   set the Non-Blocking status of the queue
 *
 * \param   messageQueue message queue
 * \param   block true to set the Non-Blocking attribute
 * \return  0 if success, -1 if error
*/
int OSAL_SetMessageQueueBlockingMode(MessageQueue_t messageQueue, bool block);

/**
 * \brief   get the number of messages that the queue can hold
 *
 * \param   messageQueue message queue
 * \return  number of messages, -1 if error
*/
int OSAL_GetMessageQueueSize(MessageQueue_t messageQueue);

/**
 * \brief   get the number of messages currently on the message queue
 *
 * \param   messageQueue message queue
 * \return  number of messages on message queue, -1 if error
*/
int OSAL_GetMessageQueueNumCurrentMessages(MessageQueue_t messageQueue);

/**
 * \brief   get the maximum sizeo of a message for this queue
 *
 * \param   messageQueue message queue
 * \return  maximum message size
*/
int OSAL_GetMessageQueueMaxMessageSize(MessageQueue_t messageQueue);

/**
 * \brief   Close the message queue
 *
 * \param   messagequeue message queue
 * \param   name message queue name
 * \return  -ve if error
*/
int OSAL_DestroyMessageQueue(MessageQueue_t messagequeue, const char * name);

/**
 * \brief   Creates a new periodic timer
 *
 * \param   callback function to be called on timer expiry
 * \param   milliseconds timer interval in milliseconds
 * \param   repeat if true, timer will re-arm after expiry.
 * \param   value argument for callback function to retrieve via OSAL_GetTimerParam()
 * \return  Timer_t handle to timer, null / 0 if error.
 */
Timer_t OSAL_NewTimer(void (*callback)(void *), uint32_t milliseconds, bool repeat, Handle_t value);

/**
 * \brief   returns val passed in to OSAL_NewTimer()
 *
 * \param   p argument passed to callback on timer expiry. 
 * \return  val passed in to OSAL_NewTimer()
 */
void * OSAL_GetTimerParam(void * p);

/**
 * \brief   Deletes the timer specified
 *
 * \param   timer timer to be deleted
 * \return  0 on success
 */
int OSAL_DestroyTimer(Timer_t timer);

/**
 * \brief   Allocate memory
 *
 * \param   size number of bytes to be allocated
 * \return  pointer to memory allocated, or NULL if allocation failed.
*/
void *malloc(size_t size);

/**
 * \brief   Free memory
 *
 * \param   ptr pointer to memory to be freed
*/
void free(void *ptr);

/**
 * \brief   Allocate and clear memory
 *
 * \param   nmemb number of elements to be allocated
 * \param   size size of each element
 * \return  pointer to memory allocated, or NULL if allocation failed.
*/
void *calloc(size_t nmemb, size_t size);

/**
 * \brief   Change size of memory block
 *
 * \param   ptr pointer to memory block to be reallocated
 * \param   size new size of memory block.
 * \return  pointer to memory allocated, or NULL if allocation failed.
*/
void *realloc(void *ptr, size_t size);

/**
 * \brief   Initialize a Mutex
 *
 * \param   mutex pointer to mutex
 * \param   attr pointer to mutex attributes
 * \return  0 on success, -1 on failure, if supported
*/
#ifdef MUTEX_TRACE
int _OSAL_InitMutex(Mutex_t * mutex, const MutexAttr_t * attr, const char * fn);
#define OSAL_InitMutex(mutex, attr) _OSAL_InitMutex(mutex, attr, __func__)
#else
int OSAL_InitMutex(Mutex_t * mutex, const MutexAttr_t * attr);
#endif

/**
 * \brief   Lock a Mutex
 *
 * \param   mutex pointer to mutex
 * \return  0 on success, or error code
*/

#ifdef MUTEX_TRACE
int _OSAL_LockMutex(Mutex_t * mutex, const char * fn);
#define OSAL_LockMutex(mutex) _OSAL_LockMutex(mutex, __func__)
#else
int OSAL_LockMutex(Mutex_t * mutex);
#endif

/**
 * \brief   Try to lock a Mutex
 *
 * \param   mutex pointer to mutex
 * \return  0 if unlocked, MUTEX_BUSY if locked, or error code
*/
#ifdef MUTEX_TRACE
int _OSAL_TryLockMutex(Mutex_t * mutex, const char * fn);
#define OSAL_TryLockMutex(mutex) _OSAL_TryLockMutex(mutex, __func__)
#else
int OSAL_TryLockMutex(Mutex_t * mutex);
#endif

/**
 * \brief   UnLock a Mutex
 *
 * \param   mutex pointer to mutex
 * \return  0 on success, or error code
*/
#ifdef MUTEX_TRACE
int _OSAL_UnLockMutex(Mutex_t * mutex, const char * fn);
#define OSAL_UnLockMutex(mutex) _OSAL_UnLockMutex(mutex, __func__)
#else
int OSAL_UnLockMutex(Mutex_t * mutex);
#endif

/**
 * \brief   Destroy a Mutex
 *
 * \param   mutex pointer to mutex
 * \return  0 on success, or error code
*/
#ifdef MUTEX_TRACE
int _OSAL_DestroyMutex(Mutex_t * mutex, const char * fn);
#define OSAL_DestroyMutex(mutex) _OSAL_DestroyMutex(mutex, __func__)
#else
int OSAL_DestroyMutex(Mutex_t * mutex);
#endif

/**
 * \brief   Log implementation, normally called using LOG_* macros,
 *          which prepend a TAG and function name to the arguments.
 *
 * prints timestamp and vararg list as formatted by printf
 *
 * \param   format - printf format string.
 * \param   ...    - varargs for printf
 */
void OSAL_Log(const char *format, ...);

/**
 * \brief   Variation of malloc that will allow us to allocate from a buffer
 *          pool (if we ever need to).
 *
 * \param   pool    Buffer pool to store in
 *
 * \param   size    Size to store
 *
 * \return          0 if failed otherwise handle to memory.
 *
 */
#ifdef MEMORY_TRACE
MemoryHandle_t _OSAL_MemoryRequest(MemoryPoolHandle_t pool, size_t size, const char * fn);
#define OSAL_MemoryRequest(pool, size) _OSAL_MemoryRequest(pool, size, __func__)
#else
MemoryHandle_t OSAL_MemoryRequest(MemoryPoolHandle_t pool, size_t size);
#endif

/**
 * \brief   Variation of free that will allow us to allocate from a buffer
 *          pool (if we ever need to).
 *
 * \param   handle  Handle of memory to de-allocate
 *
 */
#ifdef MEMORY_TRACE
void _OSAL_Free(MemoryHandle_t handle, const char * fn);
#define OSAL_Free(handle) _OSAL_Free(handle, __func__)
#else
void OSAL_Free(MemoryHandle_t handle);
#endif

/**
 * \brief   Get the size of an allocated block of memory
 *
 * \param   handle to the block size
 *
 * \return  The block size.
 */
size_t OSAL_GetBlockSize( MemoryHandle_t handle );

/**
 * \brief   Get the size of the amount stored in the memory block as opposed
 *          to the size of the block. To work it requires the amount is set
 *          on writing to the block using OSAL_SetAmountStored (below)
 *
 * \param   handle to the block size
 *
 * \return  The stored memory size.
 */
size_t OSAL_GetAmountStored( MemoryHandle_t handle );

/**
 * \brief   Used to record the amount of data has been stored in a block so it
 *          can be retrieved using OSAL_GetAmountStored. The primary intention
 *          is to re-use allocated blocks of memory rather than having to keep
 *          freeing and reallocating.
 *
 * \param   handle to the block size
 *
 * \param   newAmount the new amount
 *
 * \return  The stored memory size.
 */
size_t OSAL_SetAmountStored( MemoryHandle_t handle, size_t newAmount );

/**
 * \brief   Gets the memory block associated with a handle
 *
 * \param   handle  The handle of the block
 *
 * \return          The pointer from the handle - NULL is failure.
 */
void* OSAL_GetMemoryPointer(MemoryHandle_t handle);

/**
 * \brief   Initialise watchdog to fire after ms milliseconds
 *
 * \param   timeout_ms timeout in milliseconds
 * \return  0 on success, -1 on failure
 */
int OSAL_watchdog_init(uint32_t timeout_ms);

/**
 * \brief   Start watchdog
 *
 */
void OSAL_watchdog_start(void);

/**
 * \brief   Stop watchdog
 *
 */
void OSAL_watchdog_stop(void);

/**
 * \brief   Refresh watchdog
 *
 */
void OSAL_watchdog_refresh(void);

/**
 * \brief   Suspend watchdog refresh
 *
 */

void OSAL_watchdog_suspend_refresh(void);

typedef enum
{
    READ_ONLY    = 0x01, // open for reading
    WRITE_ONLY   = 0x02, // open for writing
    READ_WRITE   = 0x03, // open for reading and writing
    WRITE_APPEND = 0x04, // open for writing at end of store
} OSAL_AccessMode_e;

/**
 * \brief   open a store
 *
 * \param storeName - name of store
 * \param accessMode - read, write, read-write, append
 * \return Handle
 */
Handle_t OSAL_StoreOpen(const char * storeName, OSAL_AccessMode_e accessMode);

/**
 * \brief write nBytes from buffer to a store previously opened by OSAL_StoreOpen.
 *
 * \param handle - handle to open store
 * \param buffer - pointer to buffer to be written
 * \param nBytes - number of bytres to be read
 * \return number of bytes written, or -ve on error
 */
int OSAL_StoreWrite(Handle_t handle, const void * buffer, size_t nBytes);

/**
 * \brief read nBytes into buffer from a store previously opened by OSAL_StoreOpen.
 *
 * \param handle - handle to open store
 * \param buffer - pointer to buffer to be written
 * \param nBytes - number of bytres to be read
 * \return number of bytes written, or -ve on error
 */
int OSAL_StoreRead(Handle_t handle, void* buffer, size_t nBytes);

/**
 * \brief closes a store previously opened by OSAL_StoreOpen.
 *
 * \param handle - handle to open store
 */
int OSAL_StoreClose( Handle_t handle);

/**
 * \brief renames oldName to newName
 *
 * \param oldName - old name of store
 * \param newName - new name of store
 * \return 0 on success
 */
int OSAL_StoreAtomicRename(const char * oldName, const char * newName);

/**
 * \brief removes store storeName
 *
 * \param storeName - name of store to remove
 * \return 0 on success
 */
int OSAL_StoreRemove(const char * storeName);

/**
 * \brief   returns the size of a store in bytes
 *
 * \param   storeName - name of store to get the size of
 * \return  size of the file in bytes
 */
int OSAL_StoreSize(const char * storeName);

/**
 * \brief   returns true if a store exists
 *
 * \param   storeName - name of store
 * \return  true if the store exists
 */
bool OSAL_StoreExist(const char * storeName);

/**
 * \brief   returns environment variable or defautValue if not found.
 *
 * \param   name environment variable name
 * \param   defaultValue value if name no found
 *
 * \return  value associated with name
 */
char * getenvDefault(char * name, char * defaultValue);

/**
 * \brief   Initialize a binary semaphore
 *
 * \param   mutex pointer to semaphore
 * \return  0 on success, -1 on failure, if supported
*/
int OSAL_InitBinarySemaphore(Semaphore_t * semaphore);

/**
 * \brief   Give a binary semaphore
 *
 * \param   semaphore pointer to semaphore
 * \return  0 on success, or error code
*/
int OSAL_GiveBinarySemaphore(Semaphore_t *semaphore);

/**
 * \brief   Take a binary semaphore
 *
 * \param   semaphore pointer to semaphore
 * \return  0 on success, or error code
*/
int OSAL_TakeBinarySemaphore(Semaphore_t *semaphore);
#endif
