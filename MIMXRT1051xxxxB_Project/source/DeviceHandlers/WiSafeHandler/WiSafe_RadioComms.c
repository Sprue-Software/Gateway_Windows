/*!****************************************************************************
*
* \file WiSafe_RadioComms.c
*
* \author James Green
*
* \brief Handle receipt and transmission of data over SPI.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

//#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
//#include <unistd.h>
////////#include <sys/ioctl.h>
#include <string.h>

#include "LOG_Api.h"
#include "ECOM_Messages.h"

#include "WiSafe_RadioComms.h"
#include "WiSafe_RadioCommsBuffer.h"
#include "Sprue/spruerm_kernel.h"
#include "Sprue/spruerm_protocol.h"
#include "WiSafe_Engine.h"
#include "WiSafe_DAL.h"
#include "WiSafe_Main.h"
#include "Sprue/SprueConfig.h"

#include "wisafe_drv.h"

static Thread_t thread;
static Mutex_t lock; // Our mutex for preventing us from TXing and RXing at the same time.

/**
 * Entry point for comms thread.
 *
 * @param arg Unused
 *
 * @return Unused
 */
static void WiSafe_RadioCommsThread(Handle_t arg)
{
    //////// [RE:wisafe]
    wisafedrv_init();
    LOG_Info("Opened WiSafe SPI Driver");

    /* Main RX loop. */
    while (true)
    {
        /* Get a buffer to work in. */
        radioCommsBuffer_t* buf = WiSafe_RadioCommsBufferBusyGet();

        //////// [RE:wisafe]
        wisafedrv_read(buf->data, &(buf->count));

        /* Pass on for processing. */
        BufferDeEscape(buf);
        WiSafe_RadioCommsBufferDump("<- RX ", buf);

        /* Send in a message to WiSafe_Main. */
        radioComms_bufferRx_st msg;
        msg.messageType = ECOM_BUFFER_RX;
        msg.buffer = buf;

        const MessagePriority_e priority = MessagePriority_high;
        if (OSAL_SendMessage(MainMessageQueue, &msg, sizeof(msg), priority) < 0)
        {
            LOG_Error("SendMessage() failed.");
        }
    }

    LOG_Warning("WiSafe_RadioCommsThread terminating");
}

/**
 * Initialisation routine for this module - must be called before
 * anything else.
 *
 */
void WiSafe_RadioCommsInit(void)
{
    /* Initialise mutex. */
    OSAL_InitMutex(&lock, NULL);

    /* Create a thread to read packets. */
    Handle_t null = NULL;
    thread = OSAL_NewThread(WiSafe_RadioCommsThread, null);

    return;
}

/**
 * Called to close this module and free resources.
 *
 */
void WiSafe_RadioCommsClose(void)
{
    if (thread != NULL)
    {
        /* Kill the thread. */
        OSAL_KillThread(thread);
    }

    OSAL_LockMutex(&lock);

    /* Close the file handle. */
    //////// [RE:wisafe]
    wisafedrv_close();

    OSAL_UnLockMutex(&lock);

    LOG_Info("RadioComms closed.");
}

/**
 * Transmit a packet out over SPI. The buffer is freed by this
 * function.
 *
 * @param buf The unescaped buffer to be transmitted.
 */
void WiSafe_RadioCommsTx(radioCommsBuffer_t* buf)
{
    radioCommsBuffer_t* escaped = BufferEscape(buf);

    /* Add the packet terminator, and transmit. We have to put everything together in
       a single write because the underlying driver can't handle it any other way. */
    if ((escaped != NULL) && (WiSafe_RadioCommsBufferRemainingSpace(buf) >= 1))
    {
        escaped->data[escaped->count++] = SPRUERM_TERMINATING_FLAG;

        /* Wait for the mutex, to check we are not RXing and TXing at the same time. */
        OSAL_LockMutex(&lock);

        //////// [RE:wisafe]
        wisafedrv_write(escaped->data, escaped->count);

        OSAL_UnLockMutex(&lock);

        WiSafe_RadioCommsBufferDump("-> TX ", buf);
    }
    else
    {
        LOG_Error("TX overflow, not sending packet.");
        WiSafe_RadioCommsBufferDump("Dropped ", buf);
    }

    WiSafe_RadioCommsBufferRelease(escaped);
}
