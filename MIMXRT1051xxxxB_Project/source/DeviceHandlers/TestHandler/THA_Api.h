#ifndef __THA_API
#define __THA_API

/*!****************************************************************************
 *
 * \file THA_Api.h
 *
 * \author Gavin Dolling
 *
 * \brief Interface to the test handler
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "LSD_Types.h"

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e THA_Init(void);

EnsoErrorCode_e THA_Start(void);

#endif
