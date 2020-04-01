/*!****************************************************************************
*
* \file APP_Init.h
*
* \author Murat Cakmak
*
* \brief Application Initialisation Interface
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#ifndef _APP_INIT_H_
#define _APP_INIT_H_

#include "LSD_Types.h"

/******************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e APP_Initialise(char* awsIOTRootCAFilename,
							   char* awsIOTCertificateFilename,
							   char* awsIOTPrivateKeyFilename,
							   EnsoDeviceId_t* gatewayId);

#endif /* _APP_INIT_H_ */
