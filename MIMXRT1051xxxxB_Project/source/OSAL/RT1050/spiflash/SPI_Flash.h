#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

/*!****************************************************************************
 *
 * \file SPI_Flash.h
 *
 * \author Guy Bellini
 *
 * \brief SPI Flash device interface
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <SPI_Flash.issi_32mbit.h>

/* Flash layout */
#define FLASH_IMAGE_OFFSET                  0x0000      // ...past FlexSPI_AMBA_BASE
#define FLASH_IMAGE_MAX_SIZE                512 * 1024  // 512 KB
#define FLASH_IMAGE_NUM_SECTORS             FLASH_IMAGE_MAX_SIZE / FLASH_SECTOR_SIZE

#if 0
/* Flash Region for Filesystem Storage */
#if defined SPI_FLASH_TYPE_W25Q80DV
/* W25Q80DV Flash Details */
#define FLASH_SIZE                          0x100000    // 1 MB
#define FLASH_SECTOR_SIZE                   0x1000      // 4K bytes
#define FLASH_PAGE_SIZE                     0x100       // 256 bytes
#define FLASH_FS_OFFSET                     0xBF000     // ...past FlexSPI_AMBA_BASE
#define FLASH_FS_SIZE                       0x40000     // In bytes
#elif defined SPI_FLASH_TYPE_S26KS512
/* S26KS512 Flash Details */
#define FLASH_SIZE                          0x4000000   // 64 MB
#define FLASH_SECTOR_SIZE                   0x40000     // 256K bytes
#define FLASH_PAGE_SIZE                     0x200       // 512 bytes
#define FLASH_FS_OFFSET                     0x20C0000   // ...past FlexSPI_AMBA_BASE
#define FLASH_FS_SIZE                       0x1000000   // In bytes
#else
#error "Unsupported or no flash type defined"
#endif
#define FLASH_PARAM_START                   FLASH_SIZE - FLASH_SECTOR_SIZE  // ...past FlexSPI_AMBA_BASE
#endif

void SPI_Flash_Init(void);
int32_t SPI_Flash_Read(uint32_t address, uint8_t* data, uint32_t dataLen);
int32_t SPI_Flash_Write(uint32_t address, uint8_t* data, uint32_t dataLen);
int32_t SPI_Flash_Erase(uint32_t address);

#endif  /* __SPI_FLASH_H__ */
