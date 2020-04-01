/*!****************************************************************************
* \file KEY_Api.h
* \author Rhod Davies
*
* \brief Keystore external interface
*
* Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/


void * KEY_Open(void);
char * KEY_Get(void * h, const char * name);
void   KEY_Free(char * key);
int    KEY_Close(void * h);
int    KEY_Add(void * h, const char * name, const char * value);
int    KEY_Delete(void * h, const char * name);
void * KEY_Create(void);
int    KEY_Write(void * h);
char * KEY_List(void * h);
