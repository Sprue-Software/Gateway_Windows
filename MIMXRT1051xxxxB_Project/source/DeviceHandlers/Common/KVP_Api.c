/*!****************************************************************************
 *
 * \file KVP_Api.c
 *
 * \author Gavin Dolling
 *
 * \brief Test handler basic property manipulation
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "KVP_Api.h"
#include "LOG_Api.h"

/*!****************************************************************************
 * Constants
 *****************************************************************************/

/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

typedef enum ParseState_tag
{
    BEGIN_KEY,
    KEY,
    END_KEY,
    FOUND_KEY,
    COLON,
    BEGIN_VALUE,
    VALUE,
    VALUE_STRING,
    END_VALUE,
    DELIMIT,
    FINISHED,
    CLOSING_KEY_QUOTE,
    BEGIN_SKIP_VALUE,
    SKIPPING_VALUE,
    CLOSING_VALUE_QUOTE,
    MALFORMED,
    BEGIN_OBJECT,
    OBJECT
} ParseState_e;

/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

static bool _GetValueForKey(const char* key, const char* kvp, const char** from, const char** to);
static bool _ContainsFalse(const char* buffer);
static bool _ContainsTrue(const char* buffer);
static bool _ValidNumber(const char* buffer, bool* seenDecimalPoint);

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/*
 * \name   KVP_GetNextKey
 * \brief  Get the integer value pertaining to a particular key
 * \param  kvp        The key value pair list
 * \param  keyBuffer  Buffer to put the next key in
 *                    Will be null if key not found
 * \param  bufferSize The size of the buffer (will fail gracefully if key won't fit)
 * \return the remaining list or null if we are finished with whole string)
 */
const char* KVP_GetNextKey(const char* kvp, char* keyBuffer, uint32_t bufferSize)
{
    ParseState_e state = BEGIN_KEY;

    if (!kvp || !keyBuffer)
    {
        return NULL;
    }

    uint32_t index = 0;

    while (*kvp && (state != FINISHED) && (state != MALFORMED))
    {
        char current = *kvp;

        switch (state)
        {
            case BEGIN_KEY:
                if (current == '\"')
                {
                    state = KEY;
                }
                else if (current != ' ')
                {
                    state = MALFORMED;
                }
                break;

            case KEY:
                if (current == '\"')
                {
                    // Null terminate the key buffer
                    keyBuffer[index] = 0;
                    state = COLON;
                    break;
                }

                // Copy key to buffer
                keyBuffer[index++] = *kvp;

                if (index >= bufferSize)
                {
                    // We have overrunOverrun buffer as we need room to null terminate
                    state = MALFORMED;
                }
                break;

            case COLON:
                if (current == ':')
                {
                    state = BEGIN_VALUE;
                }
                else if (current != ' ')
                {
                    state = MALFORMED;
                }
                break;

            case BEGIN_VALUE:
                // Note that this means something that is not white space has
                // to be after the colon for this to be valid
                if (current == '\"')
                {
                    state = VALUE_STRING;
                }
                else if (current != ' ')
                {
                    state = DELIMIT;
                }
                break;

            case VALUE_STRING:
                // We are looking at a string, need to find end "
                if (current == '\"')
                {
                    state = DELIMIT;
                }
                break;

            case DELIMIT:
                if (current == ',')
                {
                    state = FINISHED;
                }
                break;

            default:
                LOG_Error("KVP coding error");
                state = MALFORMED;
                break;
        }

        kvp++;
    }

    const char* result;

    if (state == DELIMIT)
    {
        // Finished parsing kvp string, no next key
        result = 0;
    }
    else if (state == FINISHED)
    {
        // Found a key value pair, return pointer so we can iterate
        result = kvp;
    }
    else
    {
        // String was malformed in some way, reject
        LOG_Error("String to parse was malformed");
        *keyBuffer = 0;  // Indicates key error
        result = 0;      // No point trying for next key
    }

    return result;
}



/*
 * \name   KVP_GetType
 * \brief  Get the type for a value of a particular key
 * \param  key The key to look for
 * \param  kvp The key value pair list
 * \return The type or unknown if key not found or string is corrupt
 */
KVP_Types_e KVP_GetType(const char* key, const char* kvp)
{
    KVP_Types_e result = kvpUnknownType;
    char keyBuffer[KVP_MAX_ELEMENT_LENGTH];
    const char* next = kvp;

    do
    {
        next = KVP_GetNextKey(next, keyBuffer, KVP_MAX_ELEMENT_LENGTH);

        if (keyBuffer[0])
        {
            // Is this the key we are looking for?
            if (!strncmp(key, keyBuffer, KVP_MAX_ELEMENT_LENGTH))
            {
                const char* valueStart;
                const char* valueEnd;

                // It is our key, get hold of value
                if (_GetValueForKey(key, kvp, &valueStart, &valueEnd))
                {
                    // Null terminate the value for parsing
                    int length = valueEnd - valueStart;
                    char temp[length+1];
                    memcpy(temp, valueStart, length);
                    temp[length] = 0;

                    bool hasDecimalPoint = false;

                    // We have a value
                    if (*temp == '\"')
                    {
                        result = kvpString;
                    }
                    else if (_ContainsTrue(temp) || _ContainsFalse(temp))
                    {
                        result = kvpBoolean;
                    }
                    // Some sort of number, check characters
                    else if (!_ValidNumber(temp, &hasDecimalPoint))
                    {
                        result = kvpUnknownType;
                    }
                    else if (hasDecimalPoint)
                    {
                        result = kvpFloat;
                    }
                    else
                    {
                        result = kvpInt;
                    }

                    if (result == kvpUnknownType)
                    {
                        LOG_Error("Parsing error for %s", temp);
                    }

                    break;
                }
            }
        }
    }
    while (next);

    return result;
}



/*
 * \name   KVP_GetString
 * \brief  Get the string value for a particular key
 * \param  key The key to look for
 * \param  kvp The key value pair list
 * \param  the buffer to put the string in
 * \param  the size of the buffer
 * \return false if key not found or string won't fit in the buffer
 */
bool KVP_GetString(const char* key, const char* kvp, char* buffer, uint32_t bufferSize)
{
    if (!buffer)
    {
        return false;
    }

    bool result = true;
    const char* valueStart;
    const char* valueEnd;

    if (_GetValueForKey(key, kvp, &valueStart, &valueEnd))
    {
        if (*valueStart != '\"')
        {
            // Something not right with string
            buffer[0] = 0;
            result = false;
        }
        else
        {
            // Remove first "
            valueStart++;

            uint32_t index = 0;
            while ((*valueStart != '\"') && (result))
            {
                buffer[index++] = *valueStart++;
                if (index >= bufferSize)
                {
                    // We've gone off the end of the buffer
                    buffer[0] = 0;
                    result = false;
                }
            }

            // Terminate String
            buffer[index] = 0;
        }
    }
    else
    {
        result = false;
    }

    return result;
}



/*
 * \name   KVP_GetInt
 * \brief  Get the integer value pertaining to a particular key
 * \param  key   The key to search for
 * \param  kvp   The key value pair list
 * \param  value Pointer for space to return value
 * \return success or failure (key was not found or int string is corrupt)
 */
bool KVP_GetInt(const char* key, const char* kvp, int32_t* value)
{
    bool result = false;
    const char* valueStart;
    const char* valueEnd;

    if (_GetValueForKey(key, kvp, &valueStart, &valueEnd))
    {
        // Null terminate the value for parsing
        int length = valueEnd - valueStart;
        char temp[length+1];
        memcpy(temp, valueStart, length);
        temp[length] = 0;

        ////////if (sscanf(temp, "%8d", value) == 1)	//////// [RE:format]
        if (sscanf(temp, "%8ld", value) == 1)
        {
            result = true;
        }
    }

    return result;
}



/*
 * \name   KVP_GetFloat
 * \brief  Get the float value pertaining to a particular key
 * \param  key   The key to search for
 * \param  kvp   The key value pair list
 * \param  value Pointer for space to return value
 * \return success or failure (key was not found or float string is corrupt)
 */
bool KVP_GetFloat(const char* key, const char* kvp, float* value)
{
    bool result = false;
    const char* valueStart;
    const char* valueEnd;

    if (_GetValueForKey(key, kvp, &valueStart, &valueEnd))
    {
        // Null terminate the value for parsing
        int length = valueEnd - valueStart;
        char temp[length+1];
        memcpy(temp, valueStart, length);
        temp[length] = 0;

        if (sscanf(temp, "%8f", value) == 1)
        {
            result = true;
        }
    }

    return result;
}



/*
 * \name   KVP_GetBool
 * \brief  Get the boolean value pertaining to a particular key
 * \param  key   The key to search for
 * \param  kvp   The key value pair list
 * \param  value Pointer for space to return value
 * \return success or failure (key was not found or was not true nor false)
 */
bool KVP_GetBool(const char* key, const char* kvp, bool* value)
{
    bool result = false;
    const char* valueStart;
    const char* valueEnd;

    if (_GetValueForKey(key, kvp, &valueStart, &valueEnd))
    {
        // Null terminate the value for parsing
        int length = valueEnd - valueStart;
        char temp[length+1];
        memcpy(temp, valueStart, length);
        temp[length] = 0;

        if (_ContainsTrue(temp))
        {
            *value = true;
            result = true;
        }
        else if (_ContainsFalse(temp))
        {
            *value = false;
            result = true;
        }
    }

    return result;
}



/*!****************************************************************************
 * Private Functions
 *****************************************************************************/


/*
 * \name   _GetValueForKey
 * \brief  Get the value string for a key
 * \param  key   The key to search for
 * \param  kvp   The key value pair list
 * \param  from  Pointer to start of value
 * \param  to    Pointer to the end of the value
 * \return true if the key was found, false otherwise
 */
static bool _GetValueForKey(const char* key, const char* kvp, const char** from, const char** to)
{
    if (!key || !kvp || !from || !to)
    {
        return false;
    }

    ParseState_e state = BEGIN_KEY;
    uint32_t keyIndex = 0;

    while (*kvp && (state != FINISHED) && (state != MALFORMED))
    {
        char current = *kvp;

        switch (state)
        {
            case BEGIN_KEY:
                if (current == '\"')
                {
                    keyIndex = 0;
                    state = KEY;
                }
                else if (current != ' ')
                {
                    state = MALFORMED;
                }
                break;

            case KEY:
                // We are in key does it match what we are searching for?
                if (current != key[keyIndex])
                {
                    // Key doesn't match, but have we finished?
                    if ((current == '\"') && (key[keyIndex] == 0) && (keyIndex > 0))
                    {
                        // Key matches
                        state = FOUND_KEY;
                    }
                    else
                    {
                        // Not our key, skip value and try next key
                        if (current == '\"')
                        {
                            state = BEGIN_SKIP_VALUE;
                        }
                        else
                        {
                            state = CLOSING_KEY_QUOTE;
                        }
                    }
                } // else key is matching, carry on to next character
                keyIndex++;
                break;

            case FOUND_KEY:
                // Look for colon
                if (current == ':')
                {
                    state = BEGIN_VALUE;
                }
                else if (current != ' ')
                {
                    state = MALFORMED;
                }
                break;

            case BEGIN_VALUE:
                if (current == '\"')
                {
                    *from = kvp;
                    state = VALUE_STRING;
                }
                else if (current != ' ')
                {
                    *from = kvp;
                    state = VALUE;
                }
                break;

            case VALUE_STRING:
                if (current == '\"')
                {
                    state = VALUE;
                }
                break;

            case VALUE:
                if ((current == ',') || (current == ' '))
                {
                    *to = kvp;
                    state = FINISHED;
                }
                break;

            case CLOSING_KEY_QUOTE:
                // Need to find closing quote for key
                if (current == '\"')
                {
                    state = BEGIN_SKIP_VALUE;
                }
                break;

            case BEGIN_SKIP_VALUE:
                // Need to find colon to detect start of value to skip
                if (current == ':')
                {
                    state = SKIPPING_VALUE;
                }
                break;

            case SKIPPING_VALUE:
                if (current == '\"')
                {
                    state = CLOSING_VALUE_QUOTE;
                }
                else if (current != ' ')
                {
                    state = DELIMIT;
                }
                break;

            case CLOSING_VALUE_QUOTE:
                if (current == '\"')
                {
                    state = DELIMIT;
                }
                break;

            case DELIMIT:
                if (current == ',')
                {
                    state = BEGIN_KEY;
                }
                break;

            default:
                LOG_Error("KVP coding error");
                return 0;
        }

        kvp++;
    }

    bool result = false;

    if ((*kvp == 0) && (state == VALUE) && (*from != kvp))
    {
        // We ran off the string while we were parsing a value so terminate and return
        *to = kvp;
        result = true;
    }
    else if (state == FINISHED)
    {
        result = true;
    }

    return result;
}



/*
 * \name   _ContainsTrue
 * \brief  Test if the string contains true keyword
 * \param  buffer  string buffer
 * \return true if buffer does contain true
 */
static bool _ContainsTrue(const char* buffer)
{
    bool result = false;

    if (buffer)
    {
        if(((buffer[0] == 't') || (buffer[0] == 'T')) &&
           ((buffer[1] == 'r') || (buffer[1] == 'R')) &&
           ((buffer[2] == 'u') || (buffer[2] == 'U')) &&
           ((buffer[3] == 'e') || (buffer[3] == 'E')) &&
            (buffer[4] == 0))
        {
            result = true;
        }
    }

    return result;
}



/*
 * \name   _ContainsFalse
 * \brief  Test if the string contains false keyword
 * \param  buffer  string buffer
 * \return true if buffer does contain false
 */
static bool _ContainsFalse(const char* buffer)
{
    bool result = false;;

    if (buffer)
    {
        if (((buffer[0] == 'f') || (buffer[0] == 'F')) &&
            ((buffer[1] == 'a') || (buffer[1] == 'A')) &&
            ((buffer[2] == 'l') || (buffer[2] == 'L')) &&
            ((buffer[3] == 's') || (buffer[3] == 'S')) &&
            ((buffer[4] == 'e') || (buffer[4] == 'E')) &&
             (buffer[5] == 0))
        {
            result = true;
        }
    }

    return result;
}


/*
 * \name   _ValidNumber
 * \brief  Does the string look like a valid number
 * \param  buffer           string buffer
 * \param  seenDecimalPoint set to true if it looks like a float
 * \return true if buffer does look like a number
 */
bool _ValidNumber(const char* buffer, bool* seenDecimalPoint)
{
    bool valid = true;
    bool seenNumber = false;
    bool seenNegative = false;

    if (!buffer)
    {
        return false;
    }

    while (valid && (*buffer))
    {
        switch (*buffer++)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                seenNumber = true;
                break;

            case '-':
                if (seenNumber)
                {
                    valid = false;  // Not valid to have number then negative
                }

                if (*seenDecimalPoint)
                {
                    valid = false;  // Not valid to have decimal point then negative
                }
                else if (seenNegative)
                {
                    valid = false;  // Two negatives?
                }
                else
                {
                    seenNegative = true;
                }
                break;

            case '.':
                if (!seenNumber)
                {
                    valid = false;  // Want to see number before decimal point
                }
                else if (*seenDecimalPoint)
                {
                    valid = false;  // Can't have two decimal points
                }
                else
                {
                    *seenDecimalPoint = true;
                }
                break;

            default:
                valid = false; // Invalid character
                break;
        }
    }

    return valid;
}


/*
 * \name   GetArrayObject
 * \brief  Get the next json object from and array
 * \param  array_start start of the search
 * \param  from  Pointer to start of object
 * \param  to    Pointer to the end of the object
 * \return true if an object was found, false otherwise
 */
bool GetArrayObject(char* array_start, char** from, char** to)
{
    if (!array_start || !from || !to)
    {
        return false;
    }

    ParseState_e state = BEGIN_OBJECT;
    char *array = array_start;

    while (*array && (state != FINISHED) && (state != MALFORMED))
    {
        char current = *array;

        switch (state)
        {
            case BEGIN_OBJECT:
                if (current == '{')
                {
                    // Found start of an object - skip the opening curly bracket
                    *from = array + 1;
                    state = OBJECT;
                }
                else if (current == ']')
                {
                    // End of the array
                    state = FINISHED;
                }
                else if (current != ' ' &&
                         current != ',' &&
                         current != '}' &&
                         current != '[')
                {
                    state = MALFORMED;
                }
                break;

            case OBJECT:
                // Looking for the end of an object
                if (current == '}')
                {
                    // Found end of an object
                    *to = array - 1;
                    state = FINISHED;
                }
                break;

            default:
                LOG_Error("KVP coding error");
                return 0;
        }

        array++;
    }

    bool result = false;

    if ((*array == 0) && (state == BEGIN_OBJECT))
    {
        // We ran off the string while we were parsing the array and there
        // were no objects so terminate and return as this is not an error
        // condition, just an empty array.
        result = true;
    }
    else if (state == FINISHED)
    {
        result = true;
    }

    return result;
}
