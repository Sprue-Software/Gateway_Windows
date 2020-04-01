/*!****************************************************************************
*
* \file DEV_Api.h
*
* \author Evelyne Donnaes
*
* \brief Device Handlers API
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#ifndef _DEV_API_H_
#define _DEV_API_H_


#include <stdint.h>
#include <stdbool.h>

#include "LSD_Types.h"
#include "ECOM_Api.h"


/*!****************************************************************************
 * Macros
 *****************************************************************************/
/**< Create a PACKED macro for packing message bodies */
#ifdef __GNUC__
#define _ATTR_PACKED_               __attribute__((packed, aligned(1)))
#else
#define _ATTR_PACKED_
#endif


/******************************************************************************
 * Type Definitions
 *****************************************************************************/

/**
 * Shadow property identifier
 */
typedef union
{
    uint32_t value;

    /**< ZigBee property identifier */
    struct
    {
        uint8_t property;
        uint8_t endpoint;
        uint16_t cluster;
    } _ATTR_PACKED_ zb;

    /** Z-Wave property identifier */
    struct
    {
        uint8_t property;
        uint8_t reserved;
        uint16_t comandClass;
    } _ATTR_PACKED_ zw;

    /** Default property identifier */
    struct
    {
        uint8_t property;
        uint8_t reserved;
        uint16_t group;
    } _ATTR_PACKED_ def;
} _ATTR_PACKED_ DEV_PropertyId_t;

/******************************************************************************
 * Public Functions
 *****************************************************************************/


#endif /* _DEV_API_H_ */
