
/*!****************************************************************************
 *
 * \file Dummy_THA_Api.c
 *
 * \author Rhod Davies
 *
 * \brief Dummy Test Handler API Implementation
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "LSD_Types.h"
/**
 * \name   THA_Init
 * \brief  Test Handler main function
 * \return EnsoErrorCode_e:      0 = success else error code
 */
EnsoErrorCode_e THA_Init(void)
{
    return eecDeviceNotSupported;
}

/**
 * \name   THA_Start
 * \brief  Test Handler main function
 * \return EnsoErrorCode_e:      0 = success else error code
 */
EnsoErrorCode_e THA_Start(void)
{
    return eecDeviceNotSupported;
}
