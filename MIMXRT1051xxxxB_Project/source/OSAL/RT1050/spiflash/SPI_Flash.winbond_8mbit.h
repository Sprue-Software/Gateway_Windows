#ifndef __FILE_SYSTEM_DRIVER_H__
#define __FILE_SYSTEM_DRIVER_H__



#include "fsl_flexspi.h"



/******************************** CONSTANTS ***********************************/

#define WINBOND_8MBIT

// Flash map
#define FLASHMAP_IMAGE_RESET_HANDLER    0x800040E1
#define FLASHMAP_IMAGE_FLAG_SECTOR      0x0F000
#define FLASHMAP_RECOVERY_SIZE          0x10000
#define FLASHMAP_RECOVERY_OFFSET        0x10000
#define FLASHMAP_IMAGE_SIZE             0x60000
#define FLASHMAP_IMAGE_A_OFFSET         0x20000
#define FLASHMAP_IMAGE_B_OFFSET         0x80000

// Flash chip parameters
#define FLASH_SIZE                  0x100000    // 1 MB
#define FLASH_SECTOR_SIZE           0x1000
#define FLASH_PAGE_SIZE             256

// Flash region for filesystem
#define FS_FLASH_START              0xF8000     // ...past FlexSPI_AMBA_BASE
#define FS_FLASH_SIZE               0x8000      // In bytes

// Flash region for parameters
#define PARAM_FLASH_START           0xF0000     // ...past FlexSPI_AMBA_BASE

#endif  // __FILE_SYSTEM_DRIVER_H__
