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

#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
//////// [RE:platform]
////////#ifndef Ameba
////////#include <sys/ioctl.h>
////////#endif
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
#include "C:/gateway/SA3075-P0302_2100_AC_Gateway/Port/Build/x86_64/include/CUnit/Basic.h"
#include "WiSafeTest.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

static expectedPacket_t** expectedTX = NULL;
static volatile uint32_t expectedTXcurrent = 0;

/**
 * Simulate receiving a WiSafe radio packet
 *
 * @param buffer buffer containing bytes
 * @param length number of bytes in the buffer
 */
void WiSafe_InjectPacket(uint8_t* buffer, uint32_t length)
{
    /* Get a buffer to work in. */
   radioCommsBuffer_t* buf = WiSafe_RadioCommsBufferBusyGet();

   buf->count = length;

   for (uint32_t i=0; i<length; i++)
   {
       buf->data[i] = buffer[i];
   }

   buf->next = NULL;

   /* Pass on for processing. */
   // BufferDeEscape(buf);
   WiSafe_RadioCommsBufferDump("<- RX ", buf);

   /* Send in a message to WiSafe_Main. */
   radioComms_bufferRx_st msg;
   msg.messageType = ECOM_BUFFER_RX;
   msg.buffer = buf;

   const MessagePriority_e priority = MessagePriority_high;
#if 0
   if (OSAL_SendMessage(MainMessageQueue, &msg, sizeof(msg), priority) < 0)
   {
       LOG_Error("SendMessage() failed.");
   }
#endif
}


/**
 * Initialisation routine for this module - must be called before
 * anything else.
 *
 */
void WiSafe_RadioCommsInit(void)
{
}

/**
 * Called to close this module and free resources.
 *
 */
void WiSafe_RadioCommsClose(void)
{
}

/**
 * Transmit a packet out over SPI. The buffer is freed by this
 * function.
 *
 * @param buf The unescaped buffer to be transmitted.
 */
void WiSafe_RadioCommsTx(radioCommsBuffer_t* buf)
{
    WiSafe_RadioCommsBufferDump("-> TX ", buf);

    /* Check that this was the expected packet. */
    expectedPacket_t* expected = expectedTX[expectedTXcurrent];

    bool ok = true;

    CU_ASSERT(expected != NULL); // This would happen if the test generated more packets than were expected.
    ok &= (expected != NULL);

    CU_ASSERT((expected != NULL) && (expected->count == buf->count)); // Check packet count is as expected.
    ok &= ((expected != NULL) && (expected->count == buf->count));

    for (uint32_t loop = 0; loop < MIN(buf->count, expected->count); loop += 1)
    {
        CU_ASSERT((expected != NULL) && (expected->data[loop] == buf->data[loop])); // Check data is as expected.
        ok &= ((expected != NULL) && (expected->data[loop] == buf->data[loop]));
    }

    if (!ok)
    {
        LOG_Error("Unexpected packet TXed during test.");
        if (expected != NULL)
        {
            for (uint32_t loop = 0; loop < expected->count; loop += 1)
            {
                LOG_Error("%02x", expected->data[loop]);
            }
        }
    }

    /* Check that there is space to escape the buffer, and add a terminator. */
    radioCommsBuffer_t* escaped = BufferEscape(buf);
    CU_ASSERT((escaped != NULL) && (WiSafe_RadioCommsBufferRemainingSpace(buf) >= 1));

    /* Tidy. */
    WiSafe_RadioCommsBufferRelease(escaped);

    /* Move expected on to the next packet. */
    if (expected != NULL)
    {
        expectedTXcurrent += 1;
    }
}

void WiSafe_RegisterExpectedTX(expectedPacket_t** expectedPackets)
{
    expectedTX = expectedPackets;
    expectedTXcurrent = 0;
}

void WiSafe_WaitForExpectedTX(uint32_t timeoutInSeconds)
{
    /* Poll for all the expected packets to appear. */
    while ((timeoutInSeconds > 0) && (expectedTX[expectedTXcurrent] != NULL))
    {
        OSAL_sleep_ms(1 * 1000);
    }

    CU_ASSERT(timeoutInSeconds > 0); // Timed-out
}

void WiSafe_RegisterAndWaitForExpectedTX(uint32_t timeoutInSeconds, expectedPacket_t** expectedPackets)
{
    WiSafe_RegisterExpectedTX(expectedPackets);
    WiSafe_WaitForExpectedTX(timeoutInSeconds);
}
