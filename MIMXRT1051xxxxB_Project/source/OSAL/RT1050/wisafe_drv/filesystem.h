/*
 * filesystem.h
 *
 *  Created on: 5 Jun 2018
 *      Author: abenrashed
 */

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_


#define RM_CONFIG_FILENAME			"rm_config"
//#define FS_MAX_FILE_CONTENT_LEN		4000	//////// [RE:workaround]	Already defined

typedef struct
{
	uint8_t EEKey0;
	uint8_t EEKey1;
	uint8_t SID;
	uint8_t MeshKey[3];
	uint8_t SIDMap[8];
	uint8_t GateWayID[3];
	uint8_t SDModel[2];
	uint8_t CheckSum;
	uint8_t MFCT_Stamp0;
	uint8_t MFCT_Stamp1;
	uint8_t FreqFracKey0;
	uint8_t FreqFracKey1;
	uint8_t FreqFrac2;
	uint8_t FreqFrac3;
} tRadioModuleConfiguration;


#ifdef _FILESYSTEM_C_
#undef _FILESYSTEM_C_

tRadioModuleConfiguration RMCONFIG;
bool RadioConfigFileReset = false;

void Init_FileSystem(void);
void Init_RMConfigData(void);
bool fileexist(const char * fileName);
bool readRadioConfigFile(void);
void writeRadioConfigFile(void);
void createRadioConfigFile(void);

#else

extern tRadioModuleConfiguration RMCONFIG;
extern bool RadioConfigFileReset;

extern void Init_FileSystem(void);
extern void Init_RMConfigData(void);
extern bool fileexist(const char * fileName);
extern bool readRadioConfigFile(void);
extern void writeRadioConfigFile(void);
extern void createRadioConfigFile(void);

#endif 	// _FILESYSTEM_C_

#endif 	// _FILESYSTEM_H_
