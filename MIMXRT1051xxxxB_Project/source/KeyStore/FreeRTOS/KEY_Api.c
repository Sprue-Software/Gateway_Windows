/*!****************************************************************************
 * \file KEY_Api.c
 *
 * \author Murat Cakmak
 *
 * \brief Keystore external interface
 *
 * Retrieves null terminated text values by name from keystore.
 *
 * Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
*****************************************************************************/

//#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include "HAL.h"
#include "LOG_Api.h"
#include "OSAL_Debug.h"
#include "KEY_Api.h"
#include "KEY_Int.h"
#include "EFS_FileSystem.h"

/* Allowed max Key&Value count */
#define MAX_KEY_VALUE_PAIR_COUNT        32

/* File Name for Key store Table */
#define KEYSTORE_FILE_TABLE_NAME        "keyStoreFileTable.enso"

/* Key Value Pair */
typedef struct
{
    /* Key */
    char key[64];
    /*
     * Value
     * We keep "Value"s in files and "key" variable shows file path of "Value"
     * Therefore, no need to keep additional variable for value.
     */
} KeyAndValue;

/*
 * All Keys&Values
 */
static KeyAndValue keyAndValues[MAX_KEY_VALUE_PAIR_COUNT];

/*
 * \brief Returns slot index of existing key
 *
 * \param key Key Name
 *
 * \return index of key slot. -1 if key does not exist
 *
 */
////////static int32_t _GetKeySlot(char* key)	//////// [RE:type]
static int32_t _GetKeySlot(const char* key)
{
    for (int32_t i = 0; i < MAX_KEY_VALUE_PAIR_COUNT; i++)
    {
        if (strcmp(key, keyAndValues[i].key) == 0)
        {
            /* We found the key */
            return i;
        }
    }

    /* Key does not exist */
    return -1;
}

/*
 * \brief Returns whether Key exists
 *
 * \param key Key Name
 *
 * \return true if key exists
 *
 */
////////static bool _KeyIsExist(char* key)	//////// [RE:type]
static bool _KeyIsExist(const char* key)
{
    return _GetKeySlot(key) != -1;
}

/*
 * \brief Get an empty slot for new key
 *
 * \param none
 *
 * \return index of empty slot. -1 there does not exist any empty slot
 *
 */
static int32_t _GetEmptySlot(void)
{
    for (int i = 0; i < MAX_KEY_VALUE_PAIR_COUNT; i++)
    {
        if (keyAndValues[i].key[0] == '\0')
        {
            return i;
        }
    }

    /* There does not exist any available slot for new key&value */
    return -1;
}

/*
 * \brief Saves updated KeyStore Table file to persistant store
 *
 * \param none
 *
 * \return 0 if success otherwise -1
 *
 */
static int32_t _SaveKeyStoreFile(void)
{
    Handle_t handle = EFS_Open(KEYSTORE_FILE_TABLE_NAME, WRITE_ONLY);
    if (handle == NULL)
    {
        LOG_Error("Failed to create Key Store!");
        return -1;
    }

    int32_t retVal = EFS_Write(handle, keyAndValues, sizeof(keyAndValues));

    EFS_Close(handle);

    if (retVal != sizeof(keyAndValues))
    {
        LOG_Error("Failed to create Key Store!");
        return -1;
    }

    return 0;
}

/**
 * \brief   Return a handle to keystore
 *
 * \return  handle to malloced keystore.
 */
void * KEY_Create(void)
{
    LOG_Warning("Not Implemented");
    return NULL;
}

/**
 * \brief   Return a handle to the keystore
 *
 * \return  handle to keystore descriptor, NULL on error.
 */
void * KEY_Open(void)
{
    if (EFS_FileExist(KEYSTORE_FILE_TABLE_NAME))
    {
        Handle_t handle = EFS_Open(KEYSTORE_FILE_TABLE_NAME, READ_ONLY);

        /* Read previous content of KeyStore Table */
        int32_t retVal = EFS_Read(handle, keyAndValues, sizeof(keyAndValues));

        EFS_Close(handle);

        if (retVal != sizeof(keyAndValues))
        {
            LOG_Error("Failed to open Key Store!");
            return NULL;
        }
    }
    else
    {
        /* Create a clean KeyStore Table and save to persistant storage */
        memset(keyAndValues, 0, sizeof(keyAndValues));

        int retVal = _SaveKeyStoreFile();
        if (retVal != 0)
        {
            LOG_Error("Failed to create Key Store!");
            return NULL;
        }
    }

    return keyAndValues;
}

/**
 * \brief   close the keystore
 *
 * \param   handle to the keystore opened using KEY_Open.
 * \return  0 on success.
 */
int KEY_Close(void * h)
{
    /* Nothing to do */

    (void)h;
    return 0;
}

/**
 * \brief   Add a name,value pair to the keystore
 *
 * \param h    handle to keystore
 * \param name pointer to name of keystore entry
 * \param value pointer to value of keystore entry
 *
 * \return  0 for success, -ve for error
 */


int KEY_Add(void * h, const char * name, const char * value)
{
    if (_KeyIsExist(name))
    {
        LOG_Error("Key already exists!");
        return -1;
    }

    int emptySlot = _GetEmptySlot();
    if (emptySlot < 0)
    {
        LOG_Error("Failed to add Key&Value Pair. Key store is full!");
        return -1;
    }

    /* Save key */
    strcpy(keyAndValues[emptySlot].key, name);

    Handle_t fileHandle = EFS_Open(name, WRITE_ONLY);
    if (fileHandle == NULL)
    {
        LOG_Error("Failed to add Key&Value Pair. Failed to create file for value!");
        return -1;
    }

    /* Write Value to a file */
    int32_t writeLen = strlen(value) + 1;
    int32_t retVal = EFS_Write(fileHandle, value, writeLen);

    EFS_Close(fileHandle);

    if (retVal != writeLen)
    {
        LOG_Error("Failed to add Key&Value Pair. Failed to write value!");
        EFS_Remove(name);
        return -1;
    }

    /* Update KeyStore in persistant storage */
    retVal = _SaveKeyStoreFile();
    if (retVal != 0)
    {
        /*
         * We could not save the KeyStore file so lets delete file which newly
         * created to save value.
         */
        EFS_Remove(name);
        memset(keyAndValues[emptySlot].key, 0, sizeof(keyAndValues[emptySlot].key));
        LOG_Error("Failed to save Key Store!");
        return -1;
    }

    return 0;
}

/**
 * \brief   Delete a name from the keystore
 *
 * Deletes the first occurence of name in the keystore buffer,
 * and coalesces the keystore before resizing the buffer.
 * \param h    handle to keystore
 * \param name pointer to name of keystore entry
 *
 * \return  0 for success, -ve for error
 */
int KEY_Delete(void * h, const char * name)
{
    int32_t keyIndex = _GetKeySlot(name);
    if (keyIndex < 0)
    {
        LOG_Error("Key does not exist!");
        return -1;
    }

    KeyAndValue* keyAndValue = &keyAndValues[keyIndex];

    /* Clean entry from Key Store */
    memset(keyAndValue, 0, sizeof(KeyAndValue));

    /* Update Key Store also in persistant storage */
    int retVal = _SaveKeyStoreFile();
    if (retVal != 0)
    {
        LOG_Error("Failed to save Key Store!");
        return -1;
    }

    /* Delete file which keeps "Value" */
    EFS_Remove(name);

    return 0;
}

/**
 * \brief   gets the value stored in the key name.
 *
 * \param   h    - keystore handle
 * \param   name - name of the keystore element
 * \return  pointer to a malloced buffer containing a null terminated string.
 */
char * KEY_Get(void * h, const char * name)
{
    int32_t keyIndex = _GetKeySlot(name);
    if (keyIndex < 0)
    {
        LOG_Error("Key (%s) does not exist!", name);
        return NULL;
    }

    KeyAndValue* keyAndValue = &keyAndValues[keyIndex];

    int32_t valueLength = EFS_FileSize(keyAndValue->key);
    if (valueLength == 0)
    {
        LOG_Error("Failed to get value!");
        return NULL;
    }

    /* Alloc memory for Value */
    char* value = (char*)malloc(valueLength + 1);
    if (value == NULL)
    {
        LOG_Error("Memory Allocation failed!");
        return NULL;
    }

    Handle_t fileHandle = EFS_Open(keyAndValue->key, READ_ONLY);
    if (fileHandle == NULL)
    {
        LOG_Error("Failed to get value!");
        return NULL;
    }

    /* Read Value from file */
    int32_t retVal = EFS_Read(fileHandle, value, valueLength);

    EFS_Close(fileHandle);

    if (retVal != valueLength)
    {
        /* Failed to read file so free allocated memory */
        free(value);
        LOG_Error("Failed to get value!");
        return NULL;
    }

    LOG_Trace("%s size %d %p", name, valueLength, value);

    return value;
}

/**
 * \brief   frees a value retrieved via KEY_Get
 *
 * \param   value - pointer to value to be freed
 *
 */
void KEY_Free(char * value)
{
    if (value == NULL)
    {
       LOG_Trace("null");
       return;
    }

    free(value);

    LOG_Trace("");
}

/**
 * \brief   Writes keystore
 *
 * \param   h handle to keystore
 *
 */
int KEY_Write(void * h)
{
    if (h == NULL)
    {
       LOG_Trace("null handle");
       return -1;
    }
    LOG_Warning("Not Implemented");
    return 0;
}

/**
 * \brief   list the entries in the keystore
 *
 * \param   h handle to keystore
 *
 * Iterates over the keystore assembling a space separated, null terminated
 * string of the names of the items in the keystore.
 *
 * \return  malloced string, space separated list of names in the keystore
 */
char * KEY_List(void * h)
{
    char * list = "";
    KeyAndValue* keyAndValue;
    int32_t listSize = 0;

    for (int i = 0; i < MAX_KEY_VALUE_PAIR_COUNT; i++)
    {
        keyAndValue = &keyAndValues[i];
        if (keyAndValue->key[0] != '\0')
        {
            int32_t keyLen = strlen(keyAndValue->key);
            int32_t newListSize = listSize + keyLen + 1 + 1;

            if (listSize == 0)
            {
                list = malloc(newListSize);
                if (list == NULL)
                {
                    LOG_Error("Memory Allocation failed!");
                    return "";
                }
            }
            else
            {
                list = realloc(list, newListSize);
                if (list == NULL)
                {
                    LOG_Error("Memory Allocation failed!");
                    return "";
                }
            }

            sprintf(&list[listSize], "%s ", keyAndValue->key);

            listSize = newListSize;
        }
    }

    return list;
}
