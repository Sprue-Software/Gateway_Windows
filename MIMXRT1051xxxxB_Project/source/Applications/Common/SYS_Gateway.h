/*!****************************************************************************
*
* \file SYS_Gateway.h
*
* \author Evelyne Donnaes
*
* \brief System Device Interface
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#ifndef _SYS_GATEWAY_H_
#define _SYS_GATEWAY_H_

#include "LSD_Types.h"


/******************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e SYS_Initialise(void);

EnsoErrorCode_e SYS_Start(void);

void SYS_GetDeviceId(EnsoDeviceId_t* gatewayId);

bool SYS_IsGateway(EnsoDeviceId_t* deviceId);

// FWNAM may be passed in by -D flags to the compiler in the Build Environment.
// The fw name will be in the format:
//     <distribution index>_<hardware version>_<major>_<minor>_<name>-<version>
// Example:
//     1_sprue_1_36_sprue_hub_intamac_sprue_dev-2.0
// Declare a buffer big enough for the fw name property
#define FW_HWVER_LEN 10
#define FW_NAME_LEN 100
#define FW_DI_LEN 10
#define FW_MAJOR_LEN 10
#define FW_MINOR_LEN 10
#define FW_VERSION_LEN 5
#define MAX_FWNAME_LENGTH (FW_DI_LEN + FW_HWVER_LEN + FW_MAJOR_LEN + FW_MINOR_LEN + FW_NAME_LEN + FW_VERSION_LEN + 6)
extern char gw_fwnam[MAX_FWNAME_LENGTH + 1];

#endif /* _SYS_GATEWAY_H_ */
