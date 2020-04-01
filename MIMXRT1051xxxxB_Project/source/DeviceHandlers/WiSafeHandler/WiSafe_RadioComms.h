/*!****************************************************************************
*
* \file WiSafe_RadioComms.h
*
* \author James Green
*
* \brief Handle receipt and transmission of data over SPI.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _RADIOCOMMS_H_
#define _RADIOCOMMS_H_

#include "WiSafe_RadioCommsBuffer.h"

extern void WiSafe_RadioCommsInit(void);
extern void WiSafe_RadioCommsClose(void);
extern void WiSafe_RadioCommsTx(radioCommsBuffer_t* buf);
extern radioCommsBuffer_t* BufferEscape(radioCommsBuffer_t* buffer);
extern void BufferDeEscape(radioCommsBuffer_t* buffer);

typedef struct
{
    uint8_t messageType;
    radioCommsBuffer_t* buffer;
} radioComms_bufferRx_st;

#endif /* _RADIOCOMMS_H_ */
