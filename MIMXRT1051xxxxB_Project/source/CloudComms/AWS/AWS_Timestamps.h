#ifndef _AWS_TIMESTAMPS_H_
#define _AWS_TIMESTAMPS_H_

/*!****************************************************************************
*
* \file AWS_Timestamps.h
*
* \author Gavin Dolling
*
* \brief  Correct timestamps after we have got correct time
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include "LSD_Types.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/



/******************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e AWS_TimestampInit();

bool  AWS_IsTimeValid();

uint32_t AWS_GetTimeDelta();


#endif
