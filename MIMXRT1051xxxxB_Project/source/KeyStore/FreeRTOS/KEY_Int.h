/*!****************************************************************************
* \file KEY_Int.h
* \author Rhod Davies
*
* \brief Keystore internal interface
*
* Keystore implementation - internal implementation
* used by KEY_Api.c
*
* Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#define KEY_BITS 256
#define KEY_SIZE (KEY_BITS / 8)
#define AES_BLOCK_SIZE 16

typedef union
{
    uint8_t salt[KEY_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
} AES_hdr_t;

typedef struct
{
    AES_hdr_t hdr;
    uint8_t data[];
} AES_buf_t;

/**
 * \brief Keystore handle
 */
typedef struct
{
    int size;                       //< total size of keystore
    AES_buf_t * keys;               //< pointer to keystore
} KEY_Handle_t;

typedef struct
{
    uint8_t hub_id[16];             //< hub_id as obtained by HW_IOCTL_GET_HUB_ID ioctl.
 //nishi//   struct timespec timestamp;      //< modification time of keystore file
} ID_t;

