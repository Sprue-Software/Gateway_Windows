/*!****************************************************************************
* \file OSAL_Debug.h
* \author Rhod Davies
*
* \brief rudimentary debug macros / function definitions
*
* Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _OSAL_DUMP_H_
#define _OSAL_DUMP_H_
#include <stdarg.h>

#define NELEM(p) (sizeof(p) / sizeof(*p))

/**
 * \brief   dump buffer to Log
 *
<pre>
   0.500510 OSAL_dump: label
   0.500411 OSAL_dump: 0000 74 69 63 6b 09 20 20 20 30 20 20 20 20 20 30 2e 'tick....0.....0.'
   0.500420 OSAL_dump: 0010 35 30 30 32 37 37 0a                            '500277.         '
</pre>
 * \param   label description to add to dump (optional can pass in NULL)
 * \param   buf   pointer to buffer
 * \param   len   length of buffer
 */
void OSAL_dump(char * label, unsigned char * buf, unsigned int len, ...);

#endif
