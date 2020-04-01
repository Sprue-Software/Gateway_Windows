/*!****************************************************************************
*
* \file WiSafe_RadioCommsBuffer.cpp
*
* \author James Green
*
* \brief Buffer allocation from a pool.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

//#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "LOG_Api.h"
#include "WiSafe_RadioCommsBuffer.h"
#include "WiSafe_Utils.h"

static bool alreadyInitialised = false; // Whether we've already called init() once.
static radioCommsBuffer_t buffers[10]; // Our pool of buffers.
static radioCommsBuffer_t* nextFreeBuffer = NULL; // The next free buffer.
static Mutex_t lock; // Our mutex for concurrent accesses to the pool.

/**
 * This initialisation function for this module. Must be called before
 * everything else.
 *
 */
static void WiSafe_RadioCommsBufferInit(void)
{
    /* Don't init if we've already done it. */
    if (!alreadyInitialised)
    {
        alreadyInitialised = true;

        /* Initialise mutex. */
        OSAL_InitMutex(&lock, NULL);

        /* Set our pointer to the first free buffer. */
        nextFreeBuffer = &(buffers[0]);

        for (uint32_t loop = 0; loop < (COUNT_OF(buffers) - 1); loop += 1)
        {
            /* Set each buffer's next to the next buffer. */
            buffers[loop].next = &(buffers[loop + 1]);
        }

        /* Set the last buffer's next to NULL. */
        buffers[COUNT_OF(buffers) - 1].next = NULL;
    }
}

/**
 * Get the next free buffer. Can return NULL if no buffers are free.
 *
 * @return The buffer or NULL.
 */
radioCommsBuffer_t* WiSafe_RadioCommsBufferGet(void)
{
    /* Init (safe to call repeatedly as it checks to only do it once). */
    WiSafe_RadioCommsBufferInit();

    OSAL_LockMutex(&lock);

    /* Return the next free buffer. */
    radioCommsBuffer_t* result = nextFreeBuffer;
    if (result != NULL)
    {
        /* Remove from linked list. */
        nextFreeBuffer = result->next;
        result->next = NULL;
    }

    OSAL_UnLockMutex(&lock);

    return result;
}


/**
 * Get the next free buffer, busy waiting until one is free.
 * Guarantees not to return NULL.
 *
 * @return The buffer.
 */
radioCommsBuffer_t* WiSafe_RadioCommsBufferBusyGet(void)
{
    radioCommsBuffer_t* result;
    uint32_t check = 0;

    do
    {
        result = WiSafe_RadioCommsBufferGet();
        if (result == NULL)
        {
            OSAL_sleep_ms(1000);

            check += 1;
            if ((check % RADIOCOMMSBUFFER_MAX_BUSY_WAIT) == 0) {
                LOG_Warning("RadioCommsBuffer_busy_get() has been waiting for %d seconds.", RADIOCOMMSBUFFER_MAX_BUSY_WAIT);
            }
        }
    } while (result == NULL);

    return result;
}


/**
 * Return the given buffer to the free pool.
 *
 * @param buffer The buffer to be freed.
 */
void WiSafe_RadioCommsBufferRelease(radioCommsBuffer_t* buffer)
{
    if (buffer != NULL)
    {
        OSAL_LockMutex(&lock);

        /* Put this buffer back on the free list. */
        buffer->next = nextFreeBuffer;
        nextFreeBuffer = buffer;

        OSAL_UnLockMutex(&lock);
    }
}

/**
 * Dump a description of the buffer in the log/console.
 *
 * @param logMsgPrefix A string for prefixing log output.
 * @param buffer The buffer to describe.
 */
void WiSafe_RadioCommsBufferDump(const char* logMsgPrefix, radioCommsBuffer_t* buffer)
{
    LOG_Info("%sBuffer @ 0x%08x, has %d bytes.", logMsgPrefix, buffer, buffer->count);

    char line[(3 * RADIOCOMMSBUFFER_LENGTH) + 1];
    line[0] = 0;
    for (uint32_t loop = 0; loop < buffer->count; loop += 1)
    {
        sprintf(&(line[loop * 3]), "%02x ", buffer->data[loop]);
    }

    if (line[0] != 0)
    {
        /*
         * Add spaces to align text.
         * We could use %*c to add spaces, but it is not portable and does not
         * work with some platforms (e.g. Ameba)
         */
        char spaces[64] = { 0 };
        memset(spaces, ' ', strlen(logMsgPrefix));
        spaces[strlen(logMsgPrefix)] = '\0';

        LOG_Info("%sContents: %s", spaces, line);
    }
}


/**
 * Return the number of free bytes in the buffer.
 *
 * @param buffer The buffer.
 *
 * @return The free space in bytes.
 */
uint32_t WiSafe_RadioCommsBufferRemainingSpace(radioCommsBuffer_t* buffer)
{
    if ((buffer != NULL) && (buffer->count <= RADIOCOMMSBUFFER_LENGTH))
    {
        return (RADIOCOMMSBUFFER_LENGTH - buffer->count);
    }
    else
    {
        return 0;
    }
}

/**
 * This helper function removes a buffer from a list. But leaves the
 * caller the job of freeing the buffer.
 *
 * @param list A list of buffers.
 * @param bufferToRemove The buffer to be removed.
 */
void WiSafe_RadioCommsBufferRemove(radioCommsBuffer_t** list, radioCommsBuffer_t* bufferToRemove)
{
    radioCommsBuffer_t** parentPointer = list;
    radioCommsBuffer_t* current = *parentPointer;
    while (current != NULL)
    {
        if (current == bufferToRemove)
        {
            /* We've found it, so first remove it from the list. */
            *parentPointer = current->next;
            return;
        }

        /* On to the next. */
        parentPointer = &(current->next);
        current = current->next;
    }

    /* Not found. */
    return;
}

/**
 * Does the list contain the buffer?
 *
 * @param list A list of buffers.
 * @param bufferToFind The buffer to find.
 *
 * @return True if found, false if not.
 */
bool WiSafe_RadioCommsBufferContains(radioCommsBuffer_t* list, radioCommsBuffer_t* bufferToFind)
{
    radioCommsBuffer_t* current = list;
    while (current != NULL)
    {
        if (current == bufferToFind)
        {
            /* We've found it. */
            return true;
        }

        /* On to the next. */
        current = current->next;
    }

    /* Not found. */
    return false;
}
