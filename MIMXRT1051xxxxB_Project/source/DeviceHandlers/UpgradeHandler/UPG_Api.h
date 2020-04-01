/*!****************************************************************************
*
* \file UPG_Api.h
*
* \author Rhod Davies
*
* \brief LED Handler API
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#ifndef _UPG_API_H_
#define _UPG_API_H_

#include "LSD_Types.h"

typedef enum 
{
    UPGRADE_ERR_NONE                = 0,
    UPGRADE_ERR_INVALID_REQUEST     = 1,
    UPGRADE_ERR_DOWNLOAD_FAILED     = 2,
    UPGRADE_ERR_DECRYPTION_FAILED   = 3,
    UPGRADE_ERR_HASH_CHECK_FAILED   = 4,
    UPGRADE_ERR_FILE_CONTENTS_ERROR = 5
} UPG_Error_t;

typedef enum 
{
    UPGRADE_COMPLETE = 0,
    UPGRADE_STARTED  = 1,
    UPGRADE_BOOTING  = 2,
    UPGRADE_FAILED   = 3
} UPG_State_t;
   
EnsoErrorCode_e UPG_Init(void);
void UPG_setError(uint32_t error);
void UPG_setRetry(uint32_t retry);
void UPG_setState(uint32_t state);
void UPG_setStatus(uint32_t status);
void UPG_setSize(uint32_t size);

#endif /* _UPG_API_H_ */
