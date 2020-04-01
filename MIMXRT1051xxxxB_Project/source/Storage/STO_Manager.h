/*!****************************************************************************
*
* \file STO_Manager.h
*
* \author Evelyne Donnaes
*
* \brief Persistent Storage Manager API
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#ifndef _STO_MANAGER_H_
#define _STO_MANAGER_H_

#include "C:/Users/ndiwathe/Documents/MCUXpressoIDE_11.1.1_3241/workspace/EnsoAgent/source/LocalShadow/Api/LSD_Types.h"


/*!****************************************************************************
 * constants
 *****************************************************************************/

/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

typedef struct
{
    EnsoDeviceId_t            deviceId;
    EnsoAgentSidePropertyId_t propId;
    EnsoPropertyType_t        propType;
    PropertyGroup_e           propGroup;
} EnsoTag_t;


/******************************************************************************
 * Public Functions
 *****************************************************************************/

EnsoErrorCode_e STO_Init(void);

EnsoErrorCode_e STO_CreateNewLog(void);

EnsoErrorCode_e STO_WriteRecord(
        EnsoTag_t* tag,
        char cloudName[LSD_PROPERTY_NAME_BUFFER_SIZE], // Temporary, to be removed
        uint16_t length,
        EnsoPropertyValue_u* valuep);

EnsoErrorCode_e STO_CheckSizeAndConsolidate(void);

EnsoErrorCode_e STO_LocalShadowDumpComplete(const PropertyGroup_e propertyGroup);

void STO_RemoveLogs(void);

#endif /* _STO_MANAGER_H_ */
