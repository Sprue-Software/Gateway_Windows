/*!****************************************************************************
*
* \file WiSafe_RadioCommsCommon.c
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
//#include <unistd.h>
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


/**
 * Escape the buffer, (potentially) returning the result in a new
 * buffer. Can return NULL if there is insufficient space in the
 * buffer to do all the escaping.  This method takes ownership of the
 * buffer it is passed, and will handle any necessary releases. The
 * calling code therefore need only worry about releasing the escaped
 * buffer that is returned.
 *
 * @param buffer Buffer to be escaped.
 *
 * @return The escaped buffer.
 */
radioCommsBuffer_t* BufferEscape(radioCommsBuffer_t* buffer)
{
    /* The algorithm is to calculate the length of the escaped packet; check that
       there is room; then work backwards through the packet doing the escaping. */
    if (buffer != NULL)
    {
        /* First count how many bytes need escaping. */
        uint32_t escapeCount = 0;
        for (uint32_t loop = 0; loop < buffer->count; loop += 1)
        {
            uint8_t data = buffer->data[loop];
            if ((data == SPRUERM_TERMINATING_FLAG) || (data == SPRUERM_ESCAPE_BYTE))
            {
                escapeCount += 1;
            }
        }

        /* Check there is space. */
        uint32_t newCount = (buffer->count + escapeCount);
        if (newCount > RADIOCOMMSBUFFER_LENGTH)
        {
            WiSafe_RadioCommsBufferRelease(buffer);
            return NULL;
        }

        /* Now work backwards through the buffer doing the escaping. */
        uint8_t* src = &(buffer->data[buffer->count - 1]);
        uint8_t* dst = &(buffer->data[newCount - 1]);
        uint8_t* end = &(buffer->data[0]);

        while (src >= end)
        {
            uint8_t data = *(src--);

            /* See if the data is an escape byte or a regular byte. */
            if (data == SPRUERM_TERMINATING_FLAG)
            {
                *(dst--) = SPRUERM_ESCAPED_FLAG;
                *(dst--) = SPRUERM_ESCAPE_BYTE;
            }
            else if (data == SPRUERM_ESCAPE_BYTE)
            {
                *(dst--) = SPRUERM_ESCAPED_ESCAPE;
                *(dst--) = SPRUERM_ESCAPE_BYTE;
            }
            else
            {
                *(dst--) = data;
            }
        }

        /* Update the buffer count to include any escape sequences. */
        buffer->count = newCount;
    }

    return buffer;
}

/**
 * De-escape the given buffer, returning the result in a new buffer.
 *
 * @param buffer Buffer to be de-escaped.
 *
 * @return The de-escaped buffer.
 */
void BufferDeEscape(radioCommsBuffer_t* buffer)
{
    /* Work forwards through the buffer, de-escaping any escaped bytes. */
    uint8_t* src = &(buffer->data[0]);
    uint8_t* dst = &(buffer->data[0]);
    uint8_t* end = &(buffer->data[buffer->count]);

    while (src < end)
    {
        uint8_t data = *(src++);

        /* Is it an escaped byte? */
        if (data == SPRUERM_ESCAPE_BYTE)
        {
            assert(src < end);
            uint8_t escape = *(src++);
            if (escape == SPRUERM_ESCAPED_FLAG)
            {
                *(dst++) = SPRUERM_TERMINATING_FLAG;
            }
            else if (escape == SPRUERM_ESCAPED_ESCAPE)
            {
                *(dst++) = SPRUERM_ESCAPE_BYTE;
            }
            else
            {
                LOG_Error("Received unknown escape byte %02x.", escape);
                *(dst++) = escape; // All we can really do is write escape to dst.
            }
        }
        else
        {
            /* Just copy a regular byte straight across. */
            *(dst++) = data;
        }
    }

    /* Update the count in the buffer. */
    buffer->count = (dst - &(buffer->data[0]));

    return;
}
