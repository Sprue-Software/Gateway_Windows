/*!****************************************************************************
*
* \file STO_Dummy.c
*
* \author Rhod Davies
*
* \brief Dummy Store Manager
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

//#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "STO_Manager.h"
#include "C:/Users/ndiwathe/Documents/MCUXpressoIDE_11.1.1_3241/workspace/EnsoAgent/source/Logger/LOG_Api.h"
#include "C:/Users/ndiwathe/Documents/MCUXpressoIDE_11.1.1_3241/workspace/EnsoAgent/source/OSAL/OSAL_Api.h"
#include "C:/Users/ndiwathe/Documents/MCUXpressoIDE_11.1.1_3241/workspace/EnsoAgent//source/LocalShadow/Api/LSD_Api.h"
#include <LSD_PropertyStore.h> // For LSD_RemoveProperty_Safe

char * EnsoPropertyValue_u2s(char * buf, const size_t size, const EnsoPropertyType_t propType, const EnsoPropertyValue_u * value)
{
	return "dummy";
}

EnsoErrorCode_e STO_WriteRecord(
        EnsoTag_t* tag,
        char cloudName[LSD_PROPERTY_NAME_BUFFER_SIZE],
        uint16_t length,
        EnsoPropertyValue_u* valuep)
{
    return eecFunctionalityNotSupported;
}

EnsoErrorCode_e STO_LoadFromStorage(void)
{
    return eecFunctionalityNotSupported;
}

int STO_ReadRecord(
        Handle_t handle,
        EnsoTag_t* tag,
        char cloudName[LSD_PROPERTY_NAME_BUFFER_SIZE],
        uint16_t* length,
        EnsoPropertyValue_u* value)
{
    return -1;
}

EnsoErrorCode_e STO_CheckSizeAndConsolidate(void)
{
    return eecFunctionalityNotSupported;
}

EnsoErrorCode_e STO_LocalShadowDumpComplete(const PropertyGroup_e propertyGroup)
{
    return eecFunctionalityNotSupported;
}

void STO_RemoveLogs(void)
{
}

