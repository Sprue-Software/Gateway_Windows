
/*!****************************************************************************
*
* \file LSD_Types.c
*
* \author Carl Pickering
*
* \brief Functions for handling the types in the local shadow
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
//#include <inttypes.h>
#include "LOG_Api.h"
#include "LSD_Types.h"
#include "OSAL_Api.h"


/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static int _LSD_StringValueToJson(char* buffer, int bufferSize, const char* source);



/*!****************************************************************************
 * Public Functions
 *****************************************************************************/


/*
 * \name LSD_ConvertTypedDataToJsonValue
 *
 * \brief This function converts a data item to a string
 *
 * \param destBuffer The buffer to receive the string.
 *
 * \param destBufferSize The size of the buffer to receive the string
 *
 * \param bufferUsed The amount of the buffer that was used
 *
 * \param value The value to convert
 *
 * \param type The type of the data to convert
 */

EnsoErrorCode_e LSD_ConvertTypedDataToJsonValue(
        char* destBuffer,
        int destBufferSize,
        int* bufferUsed,
        const EnsoPropertyValue_u propVal,
        const EnsoValueType_e type)
{
    EnsoErrorCode_e retVal = eecNoError;

    /* Sanity checks so single line and early return */
    if ( NULL == destBuffer)
    {
        return eecNullPointerSupplied;
    }
    if ( NULL == bufferUsed)
    {
        return eecNullPointerSupplied;
    }
    if (1 >= destBufferSize)
    {
        return eecBufferTooSmall;
    }
    switch (type)
    {
    case evInt32:
        ////////*bufferUsed  = snprintf(destBuffer, destBufferSize, "%d", propVal.int32Value);	//////// [RE:format]
        *bufferUsed  = snprintf(destBuffer, destBufferSize, "%ld", propVal.int32Value);
        break;

    case evUnsignedInt32:
    case evTimestamp:
        ////////*bufferUsed  = snprintf(destBuffer, destBufferSize, "%u", propVal.uint32Value);	//////// [RE:format]
        *bufferUsed  = snprintf(destBuffer, destBufferSize, "%lu", propVal.uint32Value);
        break;

    case evBoolean:
        *bufferUsed  = snprintf(destBuffer, destBufferSize, propVal.booleanValue?"true":"false");
        break;

    case evFloat32:
        *bufferUsed  = snprintf(destBuffer, destBufferSize, "%f", propVal.float32Value);
        break;

    case evString:
    case evBlobHandle:
    {
        const char * source = type == evString ? propVal.stringValue : propVal.memoryHandle;
        // If we overrun when we escape characters this will return -1
        *bufferUsed = _LSD_StringValueToJson(destBuffer, destBufferSize, source);
        if (*bufferUsed < 0)
        {
            LOG_Error("String/blob property too large for buffer");
        }
        break;
    }

    default:
        retVal = eecConversionFailed;
        LOG_Error("Converting a blob to a string is unsupported.");
        break;
    }

    if ( *bufferUsed <= 0)
    {
        LOG_Error("buffer used = %d", *bufferUsed);
        retVal = eecConversionFailed;
    }

    return retVal;
}


EnsoErrorCode_e LSD_ConvertStringToTypedData(
        char* destBuffer,
        int destBufferSize,
        int* bufferUsed,
        EnsoPropertyValue_u *propVal,
        const EnsoValueType_e type )
{
    EnsoErrorCode_e retVal = eecNoError;


    /* Sanity checks so single line and early return */
    if ( NULL == destBuffer ) return eecNullPointerSupplied;
    if ( NULL == bufferUsed ) return eecNullPointerSupplied;
    if ( 1 >= destBufferSize ) return eecBufferTooSmall;



    if ( *bufferUsed <= 0)
    {
        LOG_Error("buffer used = %d", *bufferUsed);
        retVal = eecConversionFailed;
    }
    return retVal;
}



/*
 * \name LSD_GetThingName
 *
 * \brief This function converts a thing id to a string
 *
 * \param destBuffer The buffer to receive the string.
 *
 * \param destBufferSize The size of the buffer to receive the string
 *
 * \param bufferUsed The amount of the buffer that was used, including
 *                   the terminating null byte
 *
 * \param deviceId The deviceId to convert
 */

EnsoErrorCode_e LSD_GetThingName(
        char* destBuffer,
        int destBufferSize,
        int* bufferUsed,
        const EnsoDeviceId_t deviceId,
        const EnsoDeviceId_t parentId)
{
    const int thingNameLength = (2 * (sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint16_t)) + 3);

    /* Sanity checks */
    if (NULL == destBuffer)
    {
        return eecNullPointerSupplied;
    }
    if (NULL == bufferUsed)
    {
        return eecNullPointerSupplied;
    }
    if (destBufferSize < thingNameLength)
    {
        return eecBufferTooSmall;
    }

    if (deviceId.isChild)
    {
    	//nishi
#if 0
        *bufferUsed = OSAL_snprintf(destBuffer, destBufferSize, "%016"PRIx64"_%04x_%02x_%016"PRIx64"_%04x_%02x",
                                    parentId.deviceAddress,
                                    parentId.technology,
                                    parentId.childDeviceId,
                                    deviceId.deviceAddress,
                                    deviceId.technology,
                                    deviceId.childDeviceId) + 1; /* Include the terminating null byte */
#endif
    }
    else
    {
#if 0 //nishi
        *bufferUsed = OSAL_snprintf(destBuffer, destBufferSize, "%016"PRIx64"_%04x_%02x",
                                    deviceId.deviceAddress,
                                    deviceId.technology,
                                    deviceId.childDeviceId) + 1; /* Include the terminating null byte */
#endif
    }

    return eecNoError;
}


/**
 * \name    LSD_GetThingFromNameString
 *
 * \brief   This function parses the thing string to get the deviceId out of it.
 *
 * \param   sourceBuffer    The buffer containing the string to parse.
 *
 * \param   rxThing         Pointer to a thing structure to receive the thing
 *                          details
 * \return  Error Code
 */
EnsoErrorCode_e LSD_GetThingFromNameString(
        const char* sourceBuffer,
        EnsoDeviceId_t *rxThing)
{
    /* Sanity checks */
    if (!sourceBuffer || !rxThing)
    {
        return eecNullPointerSupplied;
    }

////////#ifdef Ameba	//////// [RE:platform]
#if 1
    /*
     * String formatting standard library functions in Ameba SDK do not work
     * for example for 64bit integers. For now, lets evaluate them using 32bit
     * formats
     */
    {
        /* There are two formats we have to consider:
                A gateway: <mac>_<technology>_00
                A child:   <mac>_<technology>_00_<mac>_<technology>_00
        */

        /* Try the child format first. */
        if (strlen(sourceBuffer) == 49)
        {
            rxThing->isChild = true;
            sourceBuffer += 25;
        }
        else if (strlen(sourceBuffer) == 24)
        {
            rxThing->isChild = false;
        }
        else
        {
            return eecConversionFailed;
        }

        uint32_t devAddrHigh;
        uint32_t devAddrLow;

        /* Get MSB of 64bit device address */
        ////////int parsed = sscanf(sourceBuffer, "%08x", &devAddrHigh);	//////// [RE:cast]
        int parsed = sscanf(sourceBuffer, "%08x", (unsigned int *)(&devAddrHigh));
        if (parsed != 1)
        {
            return eecConversionFailed;
        }


        /* Parse LSB of 64bit device address + Technology + Device Child ID */
        parsed = sscanf(sourceBuffer + 8, "%08x_%04x_%02x",
                ////////&devAddrLow,	//////// [RE:cast]
                (unsigned int*)&devAddrLow,
                (unsigned int*)&rxThing->technology,
                (unsigned int*)&rxThing->childDeviceId);

        if (parsed != 3)
        {
            return eecConversionFailed;
        }

        rxThing->deviceAddress = (((uint64_t)devAddrHigh) << 32) | devAddrLow;

        return eecNoError;
    }
#else
    /* There are two formats we have to consider:
            A gateway: <mac>_<technology>_00
            A child:   <mac>_<technology>_00_<mac>_<technology>_00
    */

    /* Try the child format first. */
    uint64_t dummy64;
    uint32_t dummy32;
    if (sscanf(sourceBuffer, "%016"SCNx64"_%04x_00_%016"SCNx64"_%04x_%02x",
               ////////&dummy64, &dummy32,	//////// [RE:cast]
               ////////&rxThing->deviceAddress,	//////// [RE:cast]
               (long long unsigned int*)&dummy64, (unsigned int*)&dummy32,
               (long long unsigned int*)&rxThing->deviceAddress,
               (unsigned int*)&rxThing->technology,
               (unsigned int*)&rxThing->childDeviceId) == 5)
    {
        rxThing->isChild = true;
        return eecNoError;
    }
    /* If that failed, try the gatway format. */
    else if (sscanf(sourceBuffer, "%016"SCNx64"_%04x_%02x",
               ////////&rxThing->deviceAddress,	//////// [RE:cast]
               (long long unsigned int*)&rxThing->deviceAddress,
               (unsigned int*)&rxThing->technology,
               (unsigned int*)&rxThing->childDeviceId) == 3)
    {
        rxThing->isChild = false;
        return eecNoError;
    }
    /* Otherwise we failed to parse it. */
    else
    {
        return eecConversionFailed;
    }
#endif

}

/**
 * \name    LSD_EnsoErrorCode_eToString
 *
 * \brief   returns the string representation of error
 *
 * \param   error    EnsoErrorCode_e - error code
 *
 * \return  char * string representation of error
 */
char * LSD_EnsoErrorCode_eToString(EnsoErrorCode_e error)
{
    return error == eecPropertyNotFound ? "eecPropertyNotFound" :
           error == eecFunctionalityNotSupported ? "eecFunctionalityNotSupported" :
           error == eecNullPointerSupplied ? "eecNullPointerSupplied" :
           error == eecParameterOutOfRange ? "eecParameterOutOfRange" :
           error == eecPropertyValueNotObjectHandle ? "eecPropertyValueNotObjectHandle" :
           error == eecPropertyObjectHandleNotValue ? "eecPropertyObjectHandleNotValue" :
           error == eecPropertyNotCreatedDuplicateClientId ? "eecPropertyNotCreatedDuplicateClientId" :
           error == eecPropertyNotCreatedDuplicateCloudName ? "eecPropertyNotCreatedDuplicateCloudName" :
           error == eecPropertyWrongType ? "eecPropertyWrongType" :
           error == eecPropertyGroupNotSupported ? "eecPropertyGroupNotSupported" :
           error == eecEnsoObjectNotFound ? "eecEnsoObjectNotFound" :
           error == eecEnsoDeviceNotFound ? "eecEnsoDeviceNotFound" :
           error == eecSubscriberNotFound ? "eecSubscriberNotFound" :
           error == eecEnsoObjectAlreadyRegistered ? "eecEnsoObjectAlreadyRegistered" :
           error == eecBufferTooSmall ? "eecBufferTooSmall" :
           error == eecBufferTooBig ? "eecBufferTooBig" :
           error == eecConversionFailed ? "eecConversionFailed" :
           error == eecPoolFull ? "eecPoolFull" :
           error == eecTimeOut ? "eecTimeOut" :
           error == eecEnsoObjectNotCreated ? "eecEnsoObjectNotCreated" :
           error == eecLocalShadowConnectionFailedToInitialise ? "eecLocalShadowConnectionFailedToInitialise" :
           error == eecLocalShadowConnectionFailedToConnect ? "eecLocalShadowConnectionFailedToConnect" :
           error == eecLocalShadowConnectionFailedToSetAutoConnect ? "eecLocalShadowConnectionFailedToSetAutoConnect" :
           error == eecLocalShadowConnectionFailedToClose ? "eecLocalShadowConnectionFailedToClose" :
           error == eecLocalShadowConnectionPollingFailed ? "eecLocalShadowConnectionPollingFailed" :
           error == eecLocalShadowFailedSubscription ? "eecLocalShadowFailedSubscription" :
           error == eecLocalShadowDocumentFailedToInitialise ? "eecLocalShadowDocumentFailedToInitialise" :
           error == eecLocalShadowDocumentFailedFinalise ? "eecLocalShadowDocumentFailedFinalise" :
           error == eecLocalShadowDocumentFailedToSendDelta ? "eecLocalShadowDocumentFailedToSendDelta" :
           error == eecTestFailed ? "eecTestFailed" :
           error == eecInternalError ? "eecInternalError" :
           error == eecWouldBlock ? "eecWouldBlock" :
           error == eecOpenFailed ? "eecOpenFailed" :
           error == eecCloseFailed ? "eecCloseFailed" :
           error == eecReadFailed ? "eecReadFailed" :
           error == eecWriteFailed ? "eecWriteFailed" :
           error == eecRenameFailed ? "eecRenameFailed" :
           error == eecRemoveFailed ? "eecRemoveFailed" :
           error == eecDeviceNotSupported ? "eecDeviceNotSupported" :
           error == eecNoChange ? "eecNoChange" :
           error == eecNoError ? "eecNoError" : "Undefined";
}




/*
 * \brief Convert string to a valid JSON string, escaping any escaped characters
 *
 * \param       buffer       The destination buffer
 *
 * \param       bufferSize   Space in buffer
 *
 * \param       source       Our string to convert
 *
 * \return                   -1 on error or number of bytes written to buffer
 */
static int _LSD_StringValueToJson(char* buffer, int bufferSize, const char* source)
{
    // Need at least space for ""
    if (bufferSize < 2)
        return -1;
    int sourceIndex = 0;
    int bufferIndex = 0;
    // Opening JSON "
    buffer[bufferIndex++] = '\"';
    if (source) // Null maps to ""
    {
        // Need to allow space for potential of 2 characters on each pass
        while ((bufferIndex < (bufferSize-1)) && (source[sourceIndex] != 0))
        {
            switch(source[sourceIndex])
            {
            case '\b':
                buffer[bufferIndex++] = '\\';
                buffer[bufferIndex++] = 'b';
                break;

            case '\f':
                buffer[bufferIndex++] = '\\';
                buffer[bufferIndex++] = 'f';
                break;

            case '\n':
                buffer[bufferIndex++] = '\\';
                buffer[bufferIndex++] = 'n';
                break;

            case '\r':
                buffer[bufferIndex++] = '\\';
                buffer[bufferIndex++] = 'r';
                break;

            case '\t':
                buffer[bufferIndex++] = '\\';
                buffer[bufferIndex++] = 't';
                break;

            case '\"':
                buffer[bufferIndex++] = '\\';
                buffer[bufferIndex++] = '\"';
                break;

            case '\\':
                buffer[bufferIndex++] = '\\';
                buffer[bufferIndex++] = '\\';
                break;

            default:
                buffer[bufferIndex++] = source[sourceIndex];
                break;
            }
            sourceIndex++;
        }
    }
    // Closing JSON "
    if (bufferIndex < (bufferSize-1))
    {
        // Closing JSON "
        buffer[bufferIndex++] = '\"';
        buffer[bufferIndex++] = 0;
    }
    else
    {
        // We've overrun buffer
        bufferIndex = -1;
    }
    return bufferIndex;
}

/**
 * \brief   converts a EnsoValueType_e * to a string for printing etc.
 * \param type       type
 * \return           pointer to null terminated string representation of value.
 */
char * LSD_ValueType2s(EnsoValueType_e type)
{
    switch (type)
    {
    case evInt32:         return "I32 ";
    case evUnsignedInt32: return "U32 ";
    case evFloat32:       return "F32 ";
    case evBoolean:       return "Bool";
    case evString:        return "Str ";
    case evBlobHandle:    return "Blob";
    case evTimestamp:     return "Time";
    default:              return "Unk ";
    }
}

/**
 * \brief   converts a EnsoPropertyValue_u * to a string for printing etc.
 * \param buf        buffer for string
 * \param size       size of buffer
 * \param type       type of value
 * \param pval       pointer to value
 * \return           pointer to null terminated string representation of value.
 */
char * LSD_Value2s(char * buf, int size, EnsoValueType_e type, EnsoPropertyValue_u * pval)
{
    char * bp = buf;
    switch (type)
    {
    case evInt32:
        ////////snprintf(buf, size, "%d", pval->int32Value);	//////// [RE:format]
        snprintf(buf, size, "%ld", pval->int32Value);
        break;
    case evUnsignedInt32:
        ////////snprintf(buf, size, "%u", pval->uint32Value);	//////// [RE:format]
        snprintf(buf, size, "%lu", pval->uint32Value);
        break;
    case evFloat32:
        snprintf(buf, size, "%f", pval->float32Value);
        break;
    case evBoolean:
        bp = pval->booleanValue ? "true" : "false";
        break;
    case evString:
        bp = pval->stringValue;
        break;
    case evBlobHandle:
        bp = pval->memoryHandle ? pval->memoryHandle : "";
        break;
    case evTimestamp:
        ////////snprintf(buf, size, "%u", pval->timestamp.seconds);	//////// [RE:format]
        snprintf(buf, size, "%lu", pval->timestamp.seconds);
        break;
    default:
        bp = "Unknown";
    }
    return bp;
}

/**
 * \brief   converts a EnsoPropertyValue_u * to a string for printing etc.
 * \param   group
 * \return  pointer to null terminated string representation of value.
 */
char * LSD_Group2s(PropertyGroup_e group)
{
    switch (group)
    {
    case REPORTED_GROUP: return "Reported";
    case DESIRED_GROUP:  return "Desired ";
    default:             return "Unknown ";
    }
}
