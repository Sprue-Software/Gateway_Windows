/*!****************************************************************************
*
* \file WiSafe_RadioCommsBuffer.h
*
* \author James Green
*
* \brief Buffer allocation from a pool.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _RADIOCOMMSBUFFER_H_
#define _RADIOCOMMSBUFFER_H_

#include <stdint.h>
#include <stdbool.h>

#define RADIOCOMMSBUFFER_LENGTH (32)

#define RADIOCOMMSBUFFER_MAX_BUSY_WAIT (10)

#define SPRUERM_TERMINATING_FLAG 0x7e
#define SPRUERM_ESCAPE_BYTE      0x7d
#define SPRUERM_ESCAPED_FLAG     0x01
#define SPRUERM_ESCAPED_ESCAPE   0x02

typedef struct radioCommsBuffer
{
    /* The data field must come first in the structure so that code
       can use this pool library but cast the allocated buffers to
       items of their own type. For an example, see the timer
       helper. */
    uint8_t data[RADIOCOMMSBUFFER_LENGTH];

    uint32_t count;

    struct radioCommsBuffer* next;
} radioCommsBuffer_t;

extern radioCommsBuffer_t* WiSafe_RadioCommsBufferGet(void);
extern void WiSafe_RadioCommsBufferRelease(radioCommsBuffer_t* buffer);
extern radioCommsBuffer_t* WiSafe_RadioCommsBufferBusyGet(void);
extern void WiSafe_RadioCommsBufferDump(const char* logMsgPrefix, radioCommsBuffer_t* buffer);
extern uint32_t WiSafe_RadioCommsBufferRemainingSpace(radioCommsBuffer_t* buffer);
extern void WiSafe_RadioCommsBufferRemove(radioCommsBuffer_t** list, radioCommsBuffer_t* bufferToRemove);
extern bool WiSafe_RadioCommsBufferContains(radioCommsBuffer_t* list, radioCommsBuffer_t* bufferToFind);

#endif /* _RADIOCOMMSBUFFER_H_ */
