/* spruerm.h */
/* (C) SPRUE Ltd 2014 */

/** @file
 *
 * Public API for the spruerm module.
 */

#ifndef SPRUERM_SPRUERM_H_INCLUDED
#define SPRUERM_SPRUERM_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__KERNEL__)
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <stdint.h>
#endif


/* When you write() to /dev/spruerm, you write a struct msg. You
 *  then get one back.
 */
struct spruerm_msg
{
    /** Flags */
    unsigned int flags;

    /* Length of this message, including the command byte */
    int nr_bytes;
    
    /* Payload bytes */
    uint8_t payload[32];
};

#define spruerm_cmd_byte(msg) ((msg).payload[0])


#define SPRUERM_IOCTL 's'

/* Reset the RM */
#define SPRUERM_IOCTL_RESET _IOW(SPRUERM_IOCTL, 1, int)
#define SPRUERM_IOCTL_FLUSH _IOW(SPRUERM_IOCTL, 2, int)

#if defined(__cplusplus)
}
#endif

#endif

/* End file */
