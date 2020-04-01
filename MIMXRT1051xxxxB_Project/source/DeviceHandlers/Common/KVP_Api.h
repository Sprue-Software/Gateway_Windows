#ifndef __KVP_API
#define __KVP_API

/*!****************************************************************************
 *
 * \file KVP_Api.h
 *
 * \author Gavin Dolling
 *
 * \brief Key Value Pair String parser
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*!****************************************************************************
 * Constants
 *****************************************************************************/

#define KVP_MAX_ELEMENT_LENGTH 32


/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

typedef enum KPV_Types_tag
{
    kvpInt,
    kvpFloat,
    kvpString,
    kvpBoolean,
    kvpUnknownType // This means we couldn't determine the type, parsing failed
} KVP_Types_e;


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

const char* KVP_GetNextKey(const char* kvp, char* keyBuffer, uint32_t bufferSize);

KVP_Types_e KVP_GetType(const char* key, const char* kvp);

bool KVP_GetString(const char* key, const char* kvp, char* buffer, uint32_t bufferSize);
bool KVP_GetInt(const char* key, const char* kvp, int32_t* value);
bool KVP_GetFloat(const char* key, const char* kvp, float* value);
bool KVP_GetBool(const char* key, const char* kvp, bool* value);
bool GetArrayObject(char* array_start, char** from, char** to);

#endif
