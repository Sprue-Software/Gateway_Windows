/*!****************************************************************************
 *
 * \file LEDH_Dummy.c
 *
 * \author Evelyne Donnaes
 *
 * \brief Device Handlers Dummy Implementation
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "LSD_Types.h"

/**
 * \name LEDH_Initialise
 *
 * \brief Initialise the LED Handler
 *
 * \return EnsoErrorCode_e
 */
EnsoErrorCode_e LEDH_Initialise(void)
{
	return eecDeviceNotSupported;
}

