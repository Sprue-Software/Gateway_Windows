#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

/*!****************************************************************************
 *
 * \file FileSystem.h
 *
 * \author Murat Cakmak
 *
 * \brief File System Interface for Ameba Board
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

void EFS_Init(void);
////////Handle_t EFS_Open(char* fileName, OSAL_AccessMode_e accessMode);	//////// [RE:type]
Handle_t EFS_Open(const char* fileName, OSAL_AccessMode_e accessMode);
int32_t EFS_Write(Handle_t handle, const void* buffer, size_t nBytes);
int32_t EFS_Read(Handle_t handle, void* buffer, size_t nBytes);
int32_t EFS_Close(Handle_t handle);
int32_t EFS_Rename(const char * oldName, const char * newName);
int32_t EFS_Remove(const char * fileName);
int32_t EFS_FileSize(const char * fileName);
bool EFS_FileExist(const char * fileName);
void EFS_FormatDevice(void);
void EFS_List(void);	//////// [RE:added]

#endif  /* __FILE_SYSTEM_H__ */
