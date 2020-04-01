#ifndef __STO_FILE_SYSTEM_H__
#define __STO_FILE_SYSTEM_H__

/*!****************************************************************************
 *
 * \file StoFileSystem.h
 *
 * \author Murat Cakmak
 *
 * \brief File System Interface for Ameba Board
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

void STO_Driver_Init(void);
Handle_t STO_Open(const char* fileName, OSAL_AccessMode_e accessMode);
int32_t STO_Write(Handle_t handle, const void* buffer, size_t nBytes);
int32_t STO_Read(Handle_t handle, void* buffer, size_t nBytes);
int32_t STO_Close(Handle_t handle);
int32_t STO_Rename(const char * oldName, const char * newName);
int32_t STO_Remove(const char * fileName);
int32_t STO_FileSize(const char * fileName);
bool STO_FileExist(const char * fileName);
void STO_FormatDevice(void);
void STO_List(void);	//////// [RE:added]

#endif  /* __STO_FILE_SYSTEM_H__ */
