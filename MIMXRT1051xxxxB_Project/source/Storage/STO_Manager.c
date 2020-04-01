/*!****************************************************************************
*
* \file STO_Manager.c
*
* \author Evelyne Donnaes
*
* \brief Persistent Storage Manager Implementation
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stdio.h>
//#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "APP_Types.h"
#include "STO_Manager.h"
#include "LOG_Api.h"
#include "OSAL_Api.h"
#include "LSD_Api.h"
#include "LSD_PropertyStore.h" // For LSD_RemoveProperty_Safe
#include "ECOM_Messages.h"
//nishi
#define STO_DIR "\"\""

//'\"\"'"
/*!****************************************************************************
 * constants
 *****************************************************************************/

/**
 * \name STO_MAX_STORE_SIZE_IN_BYTES
 *
 * \brief The maximum size for the log. The store should hold at least 2 times the
 * local shadow.
 *
 */
#define STO_MAX_STORE_SIZE_IN_BYTES         100000

/*!****************************************************************************
 * Static variables
 *****************************************************************************/
const char STO_CurrentLog[] = STO_DIR"StoreLog_Current";
const char STO_ArchiveLog[] = STO_DIR"StoreLog_Archive";

/*!****************************************************************************
 * Static functions
 *****************************************************************************/

EnsoErrorCode_e STO_LoadFromStorage(void);

int STO_ReadRecord(
        Handle_t handle,
        EnsoTag_t* tag,
        char cloudName[LSD_PROPERTY_NAME_BUFFER_SIZE], // Temporary, to be removed
        uint16_t* length,
        EnsoPropertyValue_u* value);

static EnsoErrorCode_e _LoadFromLog(Handle_t handle);

static EnsoErrorCode_e _DumpLocalShadow(void);

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * \name    EnsoPropertyValue_u2s
 *
 * \brief   convert EnsoPropertyValue_u2s to null terminated string in buf
 *
 * \param  buf      Store handle to read the record from
 *
 * \param  size     size of buffer
 *
 * \param  propType EnsoPropertyType_t to deunion value
 *
 * \param  value     union of types representable in shadow.
 *
 * \return pointer to buffer or pointer to "null" is buf is null
 */
char * EnsoPropertyValue_u2s(char * buf, const size_t size, const EnsoPropertyType_t propType, const EnsoPropertyValue_u * value)
{
    switch (propType.valueType)
    {
    case evInt32:
        ////////snprintf(buf, size, "I32  %d", value->int32Value);	//////// [RE:format]
        snprintf(buf, size, "I32  %ld", value->int32Value);
    break;
    case evUnsignedInt32:
        ////////snprintf(buf, size, "U32  %u", value->uint32Value);	//////// [RE:format
        snprintf(buf, size, "U32  %lu", value->uint32Value);
    break;
    case evFloat32:
        snprintf(buf, size, "F32  %f", value->float32Value);
    break;
    case evBoolean:
        snprintf(buf, size, "Bool %s", value->booleanValue ? "true" : "false");
    break;
    case evString:
        snprintf(buf, size, "Str  %s", value->stringValue);
    break;
    case evBlobHandle:
        snprintf(buf, size, "Blob %p", value->memoryHandle);
    break;
    case evTimestamp:
        ////////snprintf(buf, size, "Time %d", value->timestamp.seconds);	//////// [RE:format
        snprintf(buf, size, "Time %ld", value->timestamp.seconds);
    break;
    default:
        snprintf(buf, size, "Bad Tag");
    break;
    }
    return buf ? buf : "null";
}

/**
 * \name    STO_Init
 *
 * \brief   This function initialises the storage manager
 *
 * \return  EnsoErrorCode_e
 */
EnsoErrorCode_e STO_Init(void)
{
    EnsoErrorCode_e retVal;
    LOG_Info("%s", STO_DIR);

    // Load from storage
    retVal = STO_LoadFromStorage();

    return retVal;
}


/**
 * \name    STO_WriteRecord
 *
 * \brief   Write a TLV record to the current log
 *
 * \param   tag     Pointer to Record tag
 *
 * \param   length  Record length
 *
 * \param   value   Pointer to Record value
 *
 * \return  EnsoErrorCode_e
 */
EnsoErrorCode_e STO_WriteRecord(
        EnsoTag_t* tag,
        char cloudName[LSD_PROPERTY_NAME_BUFFER_SIZE], // Temporary, to be removed
        uint16_t length,
        EnsoPropertyValue_u* valuep)
{
    // Sanity checks
    if (!tag || !valuep)
    {
        return eecNullPointerSupplied;
    }
    EnsoErrorCode_e retVal = eecNoError;
    char buf[20];

    LOG_Trace("Tag:%016"PRIx64"_%04x_%02x %08x %08x %02x %s Name:%-11.11s Len:%d Val:%s",
        tag->deviceId.deviceAddress, tag->deviceId.technology, tag->deviceId.childDeviceId,
        tag->propId, tag->propType, tag->propGroup, LSD_Group2s(tag->propGroup),
        cloudName, length, EnsoPropertyValue_u2s(buf, sizeof buf, tag->propType, valuep));

    int32_t blobsize = 0;
    MemoryHandle_t blob = NULL;
    EnsoPropertyValue_u value = *valuep;
    if (tag->propType.valueType == evBlobHandle)
    {
        if (value.memoryHandle != NULL)
        {
            blob     = value.memoryHandle;
            blobsize = OSAL_GetBlockSize(blob);
        }
        value.uint32Value = blobsize;
    }
    uint32_t buffSize = sizeof *tag + LSD_PROPERTY_NAME_BUFFER_SIZE + sizeof length + sizeof value + blobsize;
    char * buff = OSAL_MemoryRequest(NULL, buffSize);
    if (buff == NULL)
    {
       LOG_Error("malloc %d failed", buffSize);
       retVal = eecWriteFailed;
    }
    else
    {
        char * p = buff;
        memcpy(p, tag, sizeof *tag);
        p += sizeof *tag;
        memcpy(p, cloudName, LSD_PROPERTY_NAME_BUFFER_SIZE);
        p += LSD_PROPERTY_NAME_BUFFER_SIZE;
        memcpy(p, &length, sizeof length);
        p += sizeof length;
        memcpy(p, &value, sizeof value);
        p += sizeof value;
        if (blobsize != 0)
        {
            memcpy(p, blob, blobsize);
        }
        Handle_t handle = OSAL_StoreOpen(STO_CurrentLog, WRITE_APPEND);     //////// [RE:fixme] What happens when the log gets full?
        if (handle != NULL)
        {
            int size = OSAL_StoreWrite(handle, buff, buffSize);
            if (size != buffSize)
            {
                LOG_Error("Failed to write record");
                retVal = eecWriteFailed;
            }
            OSAL_StoreClose(handle);
        }
        OSAL_Free(buff);
    }
    return retVal;
}

/**
 * \name    STO_LoadFromStorage
 *
 * \brief   At startup, read the log sequentially. If power has been lost during startup
 *          there will be an archive log and the current log. The archive shall be loaded
 *          first then the current log. When the properties have been loaded, the storage
 *          manager shall go through the consolidation process.
 *
 * \return  EnsoErrorCode_e
 */
EnsoErrorCode_e STO_LoadFromStorage(void)
{
    EnsoErrorCode_e retVal = eecNoError;

    // Whether logs consolidation should be performed
    bool consolidate = false;

    // Check if an archive log exists
    if (OSAL_StoreExist(STO_ArchiveLog))
    {
        LOG_Info("%s exist", STO_ArchiveLog);

        consolidate = true; // Will need to consolidate once the local shadow has been restored

        Handle_t archiveHandle = OSAL_StoreOpen(STO_ArchiveLog, READ_ONLY);
        if (archiveHandle == NULL)
        {
            LOG_Error("Failed to open %s", STO_ArchiveLog);
        }
        else
        {
            retVal = _LoadFromLog(archiveHandle);
            if (eecNoError != retVal)
            {
                LOG_Error("Error reading from archive log %s, deleting archive", LSD_EnsoErrorCode_eToString(retVal));

                // Let's remove the archive and start from fresh.
                OSAL_StoreRemove(STO_ArchiveLog);
                consolidate = false;
            }
            else
            {
                LOG_Info("Finished loading from %s", STO_ArchiveLog);

                if (0 != OSAL_StoreClose(archiveHandle))
                {
                    LOG_Error("Failed to close %s", STO_ArchiveLog);
                    retVal = eecCloseFailed;
                }
            }
        }
    }

    // Now read from the current log
    if (OSAL_StoreExist(STO_CurrentLog))
    {
        Handle_t currentHandle = OSAL_StoreOpen(STO_CurrentLog, READ_ONLY);
        if (currentHandle == NULL)
        {
            LOG_Error("Failed to open %s", STO_CurrentLog);
        }
        else
        {
            retVal = _LoadFromLog(currentHandle);
            if (eecNoError != retVal)
            {
                // We have a corrupted log.
                LOG_Error("Error reading from current log %s, deleting current", LSD_EnsoErrorCode_eToString(retVal));

                // Let's remove the log and start from fresh.
                OSAL_StoreRemove(STO_CurrentLog);
            }
            else
            {
                LOG_Info("Finished loading from %s", STO_CurrentLog);

                if (0 != OSAL_StoreClose(currentHandle))
                {
                    LOG_Error("Failed to close %s", STO_CurrentLog);
                    retVal = eecCloseFailed;
                }
            }
        }

        // And consolidate
        if (consolidate)
        {
            LOG_Info("Consolidating stores");

            // Dump local shadow
            retVal = _DumpLocalShadow();
        }
    }
    LSD_DumpObjectStore();
    return retVal;
}

/**
 * \name    _LoadFromLog
 *
 * \brief   Sequentially read the log identified by handle and populate Local Shadow.
 *
 * \param   handle    Store log handle
 *
 * \return  EnsoErrorCode_e
 */
static EnsoErrorCode_e _LoadFromLog(Handle_t handle)
{
    EnsoErrorCode_e retVal = eecNoError;

    bool readFromLog = true;
    while (readFromLog)
    {
        EnsoTag_t tag;
        uint16_t length;
        EnsoPropertyValue_u value;
        char cloudName[LSD_PROPERTY_NAME_BUFFER_SIZE]; // Temporary, to be removed

        int size = STO_ReadRecord(handle, &tag, cloudName, &length, &value);
        if (size > 0)
        {
            // Was the device deleted? A deleted device is indicated with a property Id of zero (invalid property).
            if (0 == tag.propId)
            {

                LOG_Info("Deleting device %016"PRIx64"_%04x_%02x", tag.deviceId.deviceAddress, tag.deviceId.technology, tag.deviceId.childDeviceId);

                // We are calling an internal Local Shadow functions because we don't want to
                // send notifications to the handlers.
                retVal = LSD_RemoveAllPropertiesOfObject_Safe(&tag.deviceId);
                if (eecNoError == retVal)
                {
                    retVal = LSD_DestroyEnsoDeviceDirectly(tag.deviceId);
                }

                if (retVal < 0)
                {
                    LOG_Error("Failed to delete device %s", LSD_EnsoErrorCode_eToString(retVal));
                    readFromLog = false;
                    break; //Early exit
                }
                continue; // Next record
            }

            // Does the device exist?
            EnsoObject_t* device = LSD_FindEnsoObjectByDeviceId(&tag.deviceId);
            if (!device)
            {
                // Create the device
                device = LSD_CreateEnsoObject(tag.deviceId);
                if (device)
                {
                    // Create the device status private property.
                    EnsoPropertyValue_u deviceStatusValue[PROPERTY_GROUP_MAX];
                    deviceStatusValue[DESIRED_GROUP].uint32Value = THING_CREATED;
                    EnsoErrorCode_e result = LSD_CreateProperty(device, PROP_DEVICE_STATUS_ID, PROP_DEVICE_STATUS_CLOUD_NAME, evUnsignedInt32, PROPERTY_PRIVATE, false, false, deviceStatusValue);
                    if (result < 0)
                    {
                        LOG_Error("Failed to create device status property %s", LSD_EnsoErrorCode_eToString(result));
                    }
            
                    // Create the connection (private) property.
                    EnsoPropertyValue_u connValue[PROPERTY_GROUP_MAX];
                    connValue[DESIRED_GROUP].int32Value = -1;
                    connValue[REPORTED_GROUP].int32Value = -1;
                    result = LSD_CreateProperty( device, PROP_CONNECTION_ID, PROP_CONNECTION_ID_CLOUD_NAME, evInt32, PROPERTY_PRIVATE, false, false, connValue);
                    if (result < 0)
                    {
                        LOG_Error("Failed to create connection property %s", LSD_EnsoErrorCode_eToString(result));
                    }
                    LSD_RegisterEnsoObject(device);
                }
                else
                {
                    LOG_Error("LSD_CreateEnsoObject failed");
                    readFromLog = false;
                    break; //Early exit
                }
            }

            // Was the property deleted?
            if (0 == length)
            {
                LOG_Info("Deleting property %x", tag.propId);
                // We are calling an internal Local Shadow function because we don't want to
                // send notifications to the handlers.
                EnsoErrorCode_e retValDel = LSD_RemoveProperty_Safe(&tag.deviceId, false, tag.propId, 0, false);
                if (retValDel < 0)
                {
                    LOG_Warning("Failed to remove property %x %s", tag.propId, LSD_EnsoErrorCode_eToString(retValDel));
                }
                continue; // Next record
            }
            else if (length != sizeof(EnsoPropertyValue_u))
            {
                LOG_Error("Corrupted record: length %d instead of %d", length, sizeof(EnsoPropertyValue_u));
                readFromLog = false;
                break; // Early exit
            }
            // The property value
            EnsoPropertyValue_u nullblob = { .memoryHandle = NULL };
            EnsoPropertyValue_u propValue[PROPERTY_GROUP_MAX];
            propValue[DESIRED_GROUP]  = tag.propType.valueType == evBlobHandle ? nullblob : value;
            propValue[REPORTED_GROUP] = tag.propType.valueType == evBlobHandle ? nullblob : value;
#if VERBOSE_STORAGE_MANAGER_DEBUG
            LOG_Info("Cloud name in log %s", cloudName);
#endif
            // Does the property exist?
            EnsoProperty_t* property = LSD_GetPropertyByAgentSideId(&tag.deviceId, tag.propId);
            if (!property)
            {
                // Restore the property
                retVal = LSD_RestoreProperty(device, tag.propId, cloudName, tag.propType, propValue);
                if (retVal < 0)
                {
                    LOG_Error("Failed to restore property %11.11s %x %s", cloudName, tag.propId,
                        LSD_EnsoErrorCode_eToString(retVal));
                    readFromLog = false;
                    break; //Early exit
                }
                else
                {
                    if (tag.propType.valueType == evString)
                    {
                        LOG_Info("Restored property %s (%x) d=%s r=%s", cloudName, tag.propId,
                                propValue[0].stringValue, propValue[1].stringValue);
                    }
                    else
                    {
                        LOG_Info("Restored property %s (%x)", cloudName, tag.propId);

                    }

                    LOG_Info("   Property %s was out of sync d=%i, r=%i", cloudName,
                            tag.propType.desiredOutOfSync, tag.propType.reportedOutOfSync);
                }
            }

            EnsoPropertyDelta_t delta;
            int deltaCounter = 0;

            /*
             * Set the value of the property.
             *
             * No need to send notifications to subscribers if we read previous
             * values from database.
             */

#if VERBOSE_STORAGE_MANAGER_DEBUG
            LOG_Info("   Setting property %s with out of sync d=%i, r=%i", cloudName,
                    tag.propType.desiredOutOfSync, tag.propType.reportedOutOfSync);
#endif


            retVal = LSD_SetPropertyValueByCloudNameWithoutNotification(
                        STORAGE_HANDLER,
                        &tag.deviceId,
                        tag.propGroup,
                        cloudName,
                        value,
                        &delta, &deltaCounter);

            if (retVal == eecNoChange)
            {
                retVal = eecNoError; // no change is not an error
            }

            // Set sync state as last action
            if (!property)
            {
                property = LSD_GetPropertyByAgentSideId(&tag.deviceId, tag.propId);
            }

            if (property)
            {
                property->type.reportedOutOfSync = tag.propType.reportedOutOfSync;
                property->type.desiredOutOfSync = tag.propType.desiredOutOfSync;
            }
            else
            {
                LOG_Error("Failed to find %s property that we have just restored?", cloudName);
            }
        }
        else if (size == 0)
        {
            LOG_Info("End of log file");
            readFromLog = false;
        }
        else
        {
            LOG_Error("STO_ReadRecord failed");
            readFromLog = false;
            retVal = eecReadFailed;
        }
    }

    return retVal;
}


/**
 * \name    STO_ReadRecord
 *
 * \brief   Read the next record from the store log identified by handle.
 *
 * \param        handle    Store handle to read the record from
 *
 * \param[out]   tag       Record tag
 *
 * \param[out]   length    Record length
 *
 * \param[out]   value     Record value
 *
 * \return  EnsoErrorCode_e
 */
int STO_ReadRecord(
        Handle_t handle,
        EnsoTag_t* tag,
        char cloudName[LSD_PROPERTY_NAME_BUFFER_SIZE], // Temporary, to be removed
        uint16_t* length,
        EnsoPropertyValue_u* value)
{
    // Sanity checks
    if (!tag || !value)
    {
        return eecNullPointerSupplied;
    }
    int size = OSAL_StoreRead(handle, tag, sizeof(EnsoTag_t));
    if (size == 0)
    {
        return 0;
    }
    if (size != sizeof(EnsoTag_t))
    {
        LOG_Error("Failed to read tag, file could be corrupted");
        return eecReadFailed;
    }
    // Property name - to be removed
    size = OSAL_StoreRead(handle, cloudName, LSD_PROPERTY_NAME_BUFFER_SIZE);
    if (size == 0)
    {
        LOG_Error("Unexpected end of file reading property name");
        return eecReadFailed;
    }
    if (size != LSD_PROPERTY_NAME_BUFFER_SIZE)
    {
        LOG_Error("Wrong size %d, file could be corrupted", size);
        return eecReadFailed;
    }
    // Length
    size = OSAL_StoreRead(handle, length, sizeof(uint16_t));
    if (size == 0)
    {
        LOG_Error("Unexpected end of file reading length");
        return eecReadFailed;
    }
    if (size != sizeof(uint16_t))
    {
        LOG_Error("Wrong size %d, file could be corrupted", size);
        return eecReadFailed;
    }
    // Value
    size = OSAL_StoreRead(handle, value, sizeof(EnsoPropertyValue_u));
    if (size == 0)
    {
        LOG_Error("Unexpected end of file reading value");
        return eecReadFailed;
    }
    if (size != sizeof(EnsoPropertyValue_u))
    {
        LOG_Error("Failed to read property value, file could be corrupted");
        return eecReadFailed;
    }
    if (tag->propType.valueType == evBlobHandle)
    {
        uint32_t blobsize = value->uint32Value;
        // blob is stored as size, data
        if (blobsize != 0)
        {
            value->memoryHandle = OSAL_MemoryRequest(NULL, blobsize);
            if (value->memoryHandle != NULL)
            {
                size = OSAL_StoreRead(handle, value->memoryHandle, blobsize);
                if (blobsize != size)
                {
                    LOG_Error("blobsize %d size %d", blobsize, size);
                    OSAL_Free(value->memoryHandle);
                    value->memoryHandle = NULL;
                    return eecReadFailed;
                }
            }
            else
            {
                LOG_Error("OSAL_MemoryRequest failed");
            }
        }
        else
        {
            value->memoryHandle = NULL;
        }
    }
#if VERBOSE_STORAGE_MANAGER_DEBUG
    char buf[20];
    LOG_Trace("Tag:%016"PRIx64"_%04x_%02x %08x %08x %02x %s Name:%-11.11s Len:%d Val:%s",
             tag->deviceId.deviceAddress, tag->deviceId.technology, tag->deviceId.childDeviceId,
             tag->propId, tag->propType, tag->propGroup,
             LSD_Group2s(tag->propGroup),
             cloudName, *length,
             EnsoPropertyValue_u2s(buf, sizeof buf, tag->propType, value));
#endif
    return size;
}

/**
 * \name    STO_CheckSizeAndConsolidate
 *
 * \brief   When the maximum permissible size has been reached the store needs
 *          to be consolidated.
 *
 * \return  EnsoErrorCode_e
 */
EnsoErrorCode_e STO_CheckSizeAndConsolidate(void)
{
    EnsoErrorCode_e retVal = eecNoError;
    int size = OSAL_StoreSize(STO_CurrentLog);
    LOG_Trace("Store size : %d", size);
    if (size >= STO_MAX_STORE_SIZE_IN_BYTES)
    {
        LOG_Info("-----> Store size is %d, consolidating ...", size);
        // Check that we don't already have an archive log.
        if (OSAL_StoreExist(STO_ArchiveLog))
        {
            LOG_Warning("Archive log already exists, archive will be lost");
        }
        // Rename the current store the archive
        int success = OSAL_StoreAtomicRename(STO_CurrentLog, STO_ArchiveLog);
        if (0 != success)
        {
            LOG_Error("OSAL_StoreAtomicRename failed");
            retVal = eecRenameFailed;
        }
        else
        {
            // Create a new empty store
            Handle_t handle = OSAL_StoreOpen(STO_CurrentLog, WRITE_ONLY);
            if (handle == NULL)
            {
                LOG_Error("Failed to open %s", STO_CurrentLog);
                retVal = eecOpenFailed;
            }
            else
            {
                if (0 != OSAL_StoreClose(handle))
                {
                    LOG_Error("Failed to close %s", STO_CurrentLog);
                    retVal = eecCloseFailed;
                }
                // Dump local shadow in the current log
                retVal = _DumpLocalShadow();
            }
        }
    }
    return retVal;
}


/**
* \name    _DumpLocalShadow
*
* \brief   Dump local shadow in the log.
*
* \return  EnsoErrorCode_e
*/
static EnsoErrorCode_e _DumpLocalShadow(void)
{
    EnsoErrorCode_e retVal = eecNoError;

    MessageQueue_t destQueue = ECOM_GetMessageQueue(LSD_HANDLER);
    uint8_t id = ECOM_DUMP_LOCAL_SHADOW;

    if (OSAL_SendMessage(destQueue, &id, 1, MessagePriority_medium) < 0)
    {
        LOG_Error("OSAL_SendMessage failed %s", strerror(errno));
        retVal = eecInternalError;
    }

    return retVal;
}


/**
 * \name    STO_LocalShadowDumpComplete
 *
 * \brief   Called when the Local Shadow has been dumped into storage.
 *          This function completes the consolidation process by
 *          removing the archive log.
 *
 * \param   propertyGroup     Property group for which the dump is complete
 *
 * \return  EnsoErrorCode_e
 */
EnsoErrorCode_e STO_LocalShadowDumpComplete(const PropertyGroup_e propertyGroup)
{
    static unsigned int count = 0;
    EnsoErrorCode_e retVal = eecNoError;

    ++count;
    if (count == 2) // We wait for a dump of reported and desired
    {
        LOG_Info("***** Dump LOCAL SHADOW Complete *****");

        // Delete the archive
        int success = OSAL_StoreRemove(STO_ArchiveLog);
        if (0 != success)
        {
            LOG_Error("OSAL_StoreRemove of %s failed", STO_ArchiveLog);
            retVal = eecRemoveFailed;
        }

        // Reset count
        count = 0;
    }
    return retVal;
}


/**
 * \name    STO_RemoveLogs
 *
 * \brief   Removes Archive and Current Logs
 */
void STO_RemoveLogs(void)
{
    if (OSAL_StoreExist(STO_ArchiveLog))
    {
        OSAL_StoreRemove(STO_ArchiveLog);
    }
    if (OSAL_StoreExist(STO_CurrentLog))
    {
        OSAL_StoreRemove(STO_CurrentLog);
    }
}

