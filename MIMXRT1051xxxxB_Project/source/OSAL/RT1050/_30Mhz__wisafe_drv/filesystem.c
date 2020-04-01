/*******************************************************************************
 * \file	filesystem.c
 * \brief 	Initialisation of file system (storing to flash). Read and write to
 *  		flash. Initialisation of Wisafe-2 configuration parameters.
 * \note 	Project: 	WG2. Low cost wireless gateway
 * \author  NXP/Abdul Ben-Rashed
 ******************************************************************************/

#define _FILESYSTEM_C_

#include <stdio.h>
#include "fsl_flexspi.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_common.h"
#include "HAL.h"
#include "LOG_Api.h"
#include "OSAL_Api.h"
#include "EFS_FileSystem.h"
#include "filesystem.h"
#include "radio.h"
#include <timer.h>
#include "messages.h"
#include "logic.h"
#include "SDcomms.h"



/*******************************************************************************
 * \brief	Initialise file system in flash.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Init_FileSystem(void)
{
    // Init
    //EFS_Init();	//////// [RE:nb] We already do this during startup

	// Format
    if(!fileexist(RM_CONFIG_FILENAME))
    {
    	//EFS_FormatDevice();	//////// [RE:workaround] This will remove all other files such as certs
    	Init_RMConfigData();
    	writeRadioConfigFile();
    }
}



/*******************************************************************************
 * \brief	Checks file existence state.
 *
 * \param   * fileName.	Pointer to the file name to be checked.
 * \return	True, if file exists. False, if file does not exist.
 ******************************************************************************/

bool fileexist(const char * fileName)
{
	return EFS_FileExist(fileName);
}



/*******************************************************************************
 * \brief	Creates radio configuration file.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void createRadioConfigFile(void)
{
	 Handle_t file = EFS_Open(RM_CONFIG_FILENAME, WRITE_ONLY);
	 EFS_Close(file);
}



/*******************************************************************************
 * \brief	Writes to radio configuration file.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void writeRadioConfigFile(void)
{
	 Handle_t file = EFS_Open(RM_CONFIG_FILENAME, WRITE_ONLY);
	 EFS_Write(file, &RMCONFIG, sizeof(RMCONFIG));
	 EFS_Close(file);
}



/*******************************************************************************
 * \brief	Reads radio configuration file.
 *
 * \param   None.
 * \return	False, if file does not exist. True, if data read was successful.
 ******************************************************************************/

bool readRadioConfigFile(void)
{
	bool success = true;

    int fileLen = EFS_FileSize(RM_CONFIG_FILENAME);
    if(fileLen==0)
    {
    	success = false;
    }
    else
    {
    	Handle_t file = EFS_Open(RM_CONFIG_FILENAME, READ_ONLY);
    	EFS_Read(file, &RMCONFIG, fileLen);
    	EFS_Close(file);
    }

	return success;
}



/*******************************************************************************
 * \brief	Initialise radio configuration data.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Init_RMConfigData(void)
{
	uint8_t * wisafeCalData;
	RMCONFIG.EEKey0 = 0;
	RMCONFIG.EEKey1 = 0;
	RMCONFIG.SID = NULL_SID;
	RMCONFIG.MeshKey[0] = 0;
	RMCONFIG.MeshKey[1] = 0;
	RMCONFIG.MeshKey[2] = 0;
	for(uint8_t i=0; i<sizeof(SIDMap); i++)
	{
		RMCONFIG.SIDMap[i] = 0;
	}
	RMCONFIG.GateWayID[0] = 0;
	RMCONFIG.GateWayID[1] = 0;
	RMCONFIG.GateWayID[2] = 0;
	RMCONFIG.SDModel[0] = 0;
	RMCONFIG.SDModel[1] = 0;
	RMCONFIG.CheckSum = 0;
	RMCONFIG.MFCT_Stamp0 = 0;
	RMCONFIG.MFCT_Stamp1 = 0;
	// restore wisafe calibration data from flash
	wisafeCalData = HAL_GetWisafeCalibration();
	if (wisafeCalData)
	{
		// valid cal data from flash
		RMCONFIG.FreqFracKey0 = wisafeCalData[0];
		RMCONFIG.FreqFracKey1 = wisafeCalData[1];
		RMCONFIG.FreqFrac2 = wisafeCalData[2];
		RMCONFIG.FreqFrac3 = wisafeCalData[3];
	}
	else
	{
		// no valid cal data from flash, set to default
		RMCONFIG.FreqFracKey0 = 0;
		RMCONFIG.FreqFracKey1 = 0;
		RMCONFIG.FreqFrac2 = FREQ_CONTROL_FRAC2_DEFAULT;
		RMCONFIG.FreqFrac3 = FREQ_CONTROL_FRAC3_DEFAULT;
	}
}
