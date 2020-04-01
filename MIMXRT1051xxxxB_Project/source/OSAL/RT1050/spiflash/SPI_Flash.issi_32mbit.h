#ifndef __FILE_SYSTEM_DRIVER_H__
#define __FILE_SYSTEM_DRIVER_H__



#include "fsl_flexspi.h"



/******************************** CONSTANTS ***********************************/

#define ISSI_32MBIT

// Flash map
#define FLASHMAP_IMAGE_RESET_HANDLER    0x800040E1
#define FLASHMAP_IMAGE_FLAG_SECTOR      0x34B000
#define FLASHMAP_RECOVERY_SIZE          0x0E0000
#define FLASHMAP_RECOVERY_OFFSET        0x020000
#define FLASHMAP_IMAGE_SIZE             0x100000
#define FLASHMAP_IMAGE_A_OFFSET         0x100000
#define FLASHMAP_IMAGE_B_OFFSET         0x200000

// Flash chip parameters
#define FLASH_SIZE                      0x400000    // 4 MB
#define FLASH_SECTOR_SIZE               0x1000
#define FLASH_PAGE_SIZE                 256

// Flash region for filesystem
#define FS_FLASH_START                  0x34C000    // ...past FlexSPI_AMBA_BASE
#define FS_FLASH_SIZE                   0x64000     // In bytes
#define STO_FLASH_START                 FS_FLASH_START + FS_FLASH_SIZE    // ...past FlexSPI_AMBA_BASE
#define STO_FLASH_SIZE                  0x50000     // In bytes

// Flash region for parameters
#define PARAM_ADDR_FLASH_START          0x01C000
#define MAC_ADDR_FLASH_START            0x01C000    // ...past FlexSPI_AMBA_BASE
#define MAC_ADDR_FLASH_SIZE             0x000006    // ...past FlexSPI_AMBA_BASE
#define SERIAL_NUM_FLASH_START          0x01C010    // ...past FlexSPI_AMBA_BASE
#define SERIAL_NUM_FLASH_SIZE           0x000019    // ...past FlexSPI_AMBA_BASE
#define WISAFE_CAL_FLASH_START          0x01C030    // ...past FlexSPI_AMBA_BASE
#define WISAFE_CAL_FLASH_SIZE           0x000005    // ...past FlexSPI_AMBA_BASE
#define ROOTCA_FLASH_START              0x01D000    // ...past FlexSPI_AMBA_BASE
#define ROOTCA_FLASH_SIZE               0x001000    // ...past FlexSPI_AMBA_BASE
#define CERT_FLASH_START                0x01E000    // ...past FlexSPI_AMBA_BASE
#define CERT_FLASH_SIZE                 0x001000    // ...past FlexSPI_AMBA_BASE
#define KEY_FLASH_START                 0x01F000    // ...past FlexSPI_AMBA_BASE
#define KEY_FLASH_SIZE                  0x001000    // ...past FlexSPI_AMBA_BASE

#endif  // __FILE_SYSTEM_DRIVER_H__
