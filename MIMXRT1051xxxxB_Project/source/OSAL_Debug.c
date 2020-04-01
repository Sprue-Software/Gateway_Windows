/*!****************************************************************************
* \file OSAL_Debug.c
* \author Rhod Davies
*
* \brief rudimentary debug function definitions
*
* Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "LOG_Api.h"
#include "OSAL_Debug.h"

void OSAL_dump(char * label, unsigned char * buf, unsigned int len, ...)
{
    if (LOG_Control.trc)
    {
        if (label && *label)
        {
            va_list ap;
            char buf[256];
            va_start(ap, len);
            vsnprintf(buf, sizeof buf, label, ap);
            va_end(ap);
            LOG_Trace("%s", buf);
        }
        const int width = 16;       /**< number of bytes to display per line      */
        char hexes[width * 3 + 1];  /**< array to hold hex representation " %02x" */
        char chars[width];          /**< array of printable chars for text        */
        ////////int l;                      /**< index into hexes for sprintf             */
        int l = 0;  //////// [RE:fixed] Inited this
        int i;                      /**< index into buf for byte                  */
        ////////int offset;                 /**< offset to print at start of line         */
        int offset = 0; //////// [RE:fixed] Inited this

        for (i = 0; i < len; i++)
        {
            unsigned char c = buf[i];
            if (i % width == 0)
            {   offset = i;
                l = 0;
                memset(chars, 0, sizeof chars);
            }
            l += snprintf(&hexes[l], sizeof hexes - l, " %02x", c);
            chars[i % sizeof chars] = c == ' ' || isgraph(c) ? c : '.';
            if  (i % width == width - 1 || i == len - 1)
            {
                LOG_Trace("%04x%-*.*s'%-*.*s'",
                        offset,
                        (int)sizeof hexes, (int)sizeof hexes, hexes,
                        (int)sizeof chars, (int)sizeof chars, chars
                );
            }
        }
    }
}
