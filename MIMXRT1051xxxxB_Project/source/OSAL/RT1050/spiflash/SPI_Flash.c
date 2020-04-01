/*!****************************************************************************
 *
 * \file SPI_Flash.c
 *
 * \author Guy Bellini
 *
 * \brief Internal Flash API
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 *  Unauthorized copying of this file, via any medium is strictly prohibited
 *
 ******************************************************************************/

/****************************** INCLUDES **************************************/
#include "OSAL_Api.h"
#include "LOG_Api.h"
#include "SPI_Flash.h"

#include "fsl_flexspi.h"

#if defined ISSI_32MBIT
/******************************** CONSTANTS ***********************************/

// LUT
#define HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA     0
#define HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA    9    // NB NOT USED
#define HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS   1
#define HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE  3
#define HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR  5
#define HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM  9
#define CUSTOM_LUT_LENGTH                       48   // == 12 * 4



static const uint32_t customLUT[CUSTOM_LUT_LENGTH] =
{
    //0
    //Fast read Quad IO DTR Mode Operation in SPI Mode (normal read)
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xED, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_4PAD, 0x18),
//    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_4PAD, 0x0C, kFLEXSPI_Command_READ_DDR, kFLEXSPI_4PAD, 0x08),
//    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    // Insert mode bits into read sequence to ensure that flash device does not enter continuous read mode (see Redmine #4595)
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_MODE8_DDR, kFLEXSPI_4PAD, 0x00, kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_4PAD, 0x0A),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_4PAD, 0x08, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //1
    //Read Status
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x05, kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //2
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //3
    //Write Enable
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x06, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //4
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //5
    //Erase Sector
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xD7, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //6
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //7
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //8
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //9
    //Page Program
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x02, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x8, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //10
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //11
    //Chip Erase
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xC7, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
};

static flexspi_device_config_t deviceconfig = {
    .flexspiRootClk = 42000000, // 42MHZ SPI serial clock
    .isSck2Enabled = false,
    .flashSize = FLASH_SIZE / 1024,  // In KBytes
    .CSIntervalUnit = kFLEXSPI_CsIntervalUnit1SckCycle,
    .CSInterval = 2,
    .CSHoldTime = 0,
    .CSSetupTime = 3,
    .dataValidTime = 1,
    .columnspace = 0,
    .enableWordAddress = false,
    .AWRSeqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA,
    .AWRSeqNumber = 1,
    .ARDSeqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA,
    .ARDSeqNumber = 1,
    .AHBWriteWaitUnit = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
    .AHBWriteWaitInterval = 20,
};


/***************************** DRIVER METHODS *******************************/


static status_t flexspi_nor_write_enable(FLEXSPI_Type *base, uint32_t baseAddr)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    // Write neable
    flashXfer.deviceAddress = baseAddr;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE;

    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}



static status_t flexspi_nor_wait_bus_busy(FLEXSPI_Type *base)
{
    // Wait status ready.
    bool isBusy;
    uint32_t readValue;
    status_t status;
    flexspi_transfer_t flashXfer;

    flashXfer.deviceAddress = 0;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Read;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS;
    flashXfer.data = &readValue;
    flashXfer.dataSize = 2;

    isBusy=true;
    do
    {
        status = FLEXSPI_TransferBlocking(base, &flashXfer);

        if (status != kStatus_Success)
        {
            return status;
        }
        if (readValue & 0x0003)
        {
            isBusy = true;
        }
        else
        {
            isBusy = false;
        }

    } while (isBusy);

    return status;
}



static status_t flexspi_nor_flash_erase_sector(FLEXSPI_Type *base, uint32_t address)
{
    status_t status;
    flexspi_transfer_t flashXfer;

    // Write enable
    status = flexspi_nor_write_enable(base, address);

    if (status != kStatus_Success)
    {
        return status;
    }

    flashXfer.deviceAddress = address;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR;
    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    status = flexspi_nor_wait_bus_busy(base);

    return status;
}



static status_t flexspi_nor_flash_page_program(FLEXSPI_Type *base, uint32_t address, const uint32_t *src, uint32_t dataLen)
{
    status_t status;
    flexspi_transfer_t flashXfer;

    // Write neable
    status = flexspi_nor_write_enable(base, address);

    if (status != kStatus_Success)
    {
        return status;
    }

    // Prepare page program command
    flashXfer.deviceAddress = address;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Write;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM;
    flashXfer.data = (uint32_t *)src;
    flashXfer.dataSize = dataLen;//FLASH_PAGE_SIZE;
    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    status = flexspi_nor_wait_bus_busy(base);

    return status;
}



static void flexspi_nor_init()
{
    flexspi_config_t config;
    FLEXSPI_GetDefaultConfig(&config);
    config.ahbConfig.enableAHBPrefetch = true;
    config.enableSckBDiffOpt = true;
    config.rxSampleClock = kFLEXSPI_ReadSampleClkLoopbackFromSckPad;
    config.enableCombination = true;
    FLEXSPI_Init(FLEXSPI, &config);

    // Configure flash settings according to serial flash feature.
    FLEXSPI_SetFlashConfig(FLEXSPI, &deviceconfig, kFLEXSPI_PortA1);

    // Update LUT table.
    FLEXSPI_UpdateLUT(FLEXSPI, 0, customLUT, CUSTOM_LUT_LENGTH);

    // Do software reset.
    FLEXSPI_SoftwareReset(FLEXSPI);
}

#elif defined WINBOND_8MBIT

/******************************** CONSTANTS ***********************************/

// LUT
#define HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA     0
#define HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA    9  // NB NOT USED
#define HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS   1
#define HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE  3
#define HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR  5
#define HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM  9
#define CUSTOM_LUT_LENGTH                       60

static const uint32_t customLUT[CUSTOM_LUT_LENGTH] =
{
    //0
    //Read Data
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x03, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //1
    //Read Status
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x05, kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //2
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //3
    //Write Enable
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x06, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //4
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //5
    //Erase Sector
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x20, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //6
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //7
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //8
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //9
    //Page Program
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x02, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x8, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //10
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //11
    //Chip Erase
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xC7, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
    FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

    //12
    //Reserved
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //13
    //Reserved
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

    //14
    //Reserved
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
};

static flexspi_device_config_t deviceconfig = {
    .flexspiRootClk = 42000000, // 42MHZ SPI serial clock
    .isSck2Enabled = false,
    .flashSize = FLASH_SIZE / 1024,  // In KBytes
    .CSIntervalUnit = kFLEXSPI_CsIntervalUnit1SckCycle,
    .CSInterval = 2,
    .CSHoldTime = 0,
    .CSSetupTime = 3,
    .dataValidTime = 1,
    .columnspace = 0,
    .enableWordAddress = false,
    .AWRSeqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA,
    .AWRSeqNumber = 1,
    .ARDSeqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA,
    .ARDSeqNumber = 1,
    .AHBWriteWaitUnit = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
    .AHBWriteWaitInterval = 20,
};


/***************************** DRIVER METHODS *******************************/

static status_t flexspi_nor_write_enable(FLEXSPI_Type *base, uint32_t baseAddr)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    do
    {
        // Write enable
        flashXfer.deviceAddress = baseAddr;
        flashXfer.port = kFLEXSPI_PortA1;
        flashXfer.cmdType = kFLEXSPI_Command;
        flashXfer.SeqNumber = 1;
        flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE;

        status = FLEXSPI_TransferBlocking(base, &flashXfer);
    }
    while (status != kStatus_Success);

    return status;
}



static status_t flexspi_nor_wait_bus_busy(FLEXSPI_Type *base)
{
    // Wait status ready.
    bool isBusy;
    uint32_t readValue;
    status_t status;
    flexspi_transfer_t flashXfer;

    flashXfer.deviceAddress = 0;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Read;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS;
    flashXfer.data = &readValue;
    flashXfer.dataSize = 2;

    isBusy=true;
    do
    {
        status = FLEXSPI_TransferBlocking(base, &flashXfer);

        if (status != kStatus_Success)
        {
            return status;
        }
        if (readValue & 0x0003)
        {
            isBusy = true;
        }
        else
        {
            isBusy = false;
        }

    } while (isBusy);

    return status;
}



static status_t flexspi_nor_flash_erase_sector(FLEXSPI_Type *base, uint32_t address)
{
    // PRINTF("XXXXXXXX flexspi_nor_flash_erase_sector 0x%p\r\n", address);

    status_t status;
    flexspi_transfer_t flashXfer;

    // Write enable
    status = flexspi_nor_write_enable(base, address);

    if (status != kStatus_Success)
    {
        return status;
    }

    flashXfer.deviceAddress = address;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR;
    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    status = flexspi_nor_wait_bus_busy(base);

    return status;
}



static status_t flexspi_nor_flash_page_program(FLEXSPI_Type *base, uint32_t address, const uint32_t *src, uint32_t dataLen)
{
    // PRINTF("XXXXXXXX flexspi_nor_flash_page_program %p\r\n", address);

    status_t status;
    flexspi_transfer_t flashXfer;

    // Write eneable
    status = flexspi_nor_write_enable(base, address);

    if (status != kStatus_Success)
    {
        return status;
    }

    // Prepare page program command
    flashXfer.deviceAddress = address;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Write;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM;
    flashXfer.data = (uint32_t *)src;
    flashXfer.dataSize = dataLen;//FLASH_PAGE_SIZE;
    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    status = flexspi_nor_wait_bus_busy(base);

    return status;
}



static void flexspi_nor_init()
{
    flexspi_config_t config;
    FLEXSPI_GetDefaultConfig(&config);
    config.ahbConfig.enableAHBPrefetch = true;
    config.enableSckBDiffOpt = true;
    config.rxSampleClock = kFLEXSPI_ReadSampleClkLoopbackFromSckPad;
    config.enableCombination = true;
    FLEXSPI_Init(FLEXSPI, &config);

    // Configure flash settings according to serial flash feature.
    FLEXSPI_SetFlashConfig(FLEXSPI, &deviceconfig, kFLEXSPI_PortA1);

    // Update LUT table.
    FLEXSPI_UpdateLUT(FLEXSPI, 0, customLUT, CUSTOM_LUT_LENGTH);

    // Do software reset.
    FLEXSPI_SoftwareReset(FLEXSPI);
}

#elif defined HF_512MBIT

// LUT
#define HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA     0
#define HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA    1
#define HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS   2
#define HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE  4
#define HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR  6
#define HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM  10
#define CUSTOM_LUT_LENGTH                       48

static const uint32_t customLUT[CUSTOM_LUT_LENGTH] =
{
        // Read Data
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xA0, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA + 1] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10, kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x04),

        // Write Data
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA + 1] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10, kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x02),
        
        // Read Status
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 1] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA), // ADDR 0x555
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 2] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 3] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x70), // DATA 0x70
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 4] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xA0, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 5] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10, kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_8PAD, 0x0B),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 6] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

        // Write Enable
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 1] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA), // ADDR 0x555
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 2] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 3] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA), // DATA 0xAA
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 4] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 5] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 6] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x02),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 7] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),

        // Erase Sector 
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 1] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA), // ADDR 0x555
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 2] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 3] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x80), // DATA 0x80
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 4] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 5] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 6] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 7] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA), // ADDR 0x555
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 8] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 9] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 10] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x02),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 11] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 12] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 13] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 14] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x30, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

        // program page
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 1] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA), // ADDR 0x555
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 2] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 3] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xA0), // DATA 0xA0
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 4] =
            FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00, kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
        [4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 5] = FLEXSPI_LUT_SEQ(
            kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10, kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x80),
};

static flexspi_device_config_t deviceconfig = {
    .flexspiRootClk = 42000000, // 42MHZ SPI serial clock
    .isSck2Enabled = false,
    .flashSize = FLASH_SIZE / 1024,  // In KBytes
    .CSIntervalUnit = kFLEXSPI_CsIntervalUnit1SckCycle,
    .CSInterval = 2,
    .CSHoldTime = 0,
    .CSSetupTime = 3,
    .dataValidTime = 1,
    .columnspace = 3,
    .enableWordAddress = true,
    .AWRSeqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA,
    .AWRSeqNumber = 1,
    .ARDSeqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA,
    .ARDSeqNumber = 1,
    .AHBWriteWaitUnit = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
    .AHBWriteWaitInterval = 20,
};



/***************************** DRIVER METHODS *******************************/

static status_t flexspi_nor_write_enable(FLEXSPI_Type *base, uint32_t baseAddr)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    // Write enable
    flashXfer.deviceAddress = baseAddr;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 2;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE;

    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}



static status_t flexspi_nor_wait_bus_busy(FLEXSPI_Type *base)
{
    // Wait status ready.
    bool isBusy;
    uint32_t readValue;
    status_t status;
    flexspi_transfer_t flashXfer;

    flashXfer.deviceAddress = 0;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Read;
    flashXfer.SeqNumber = 2;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS;
    flashXfer.data = &readValue;
    flashXfer.dataSize = 2;

    do
    {
        status = FLEXSPI_TransferBlocking(base, &flashXfer);
    
        // PRINTF("XXXXXXXX flexspi_nor_wait_bus_busy STATUS = %X\r\n", status);

        if (status != kStatus_Success)
        {
            return status;
        }
        if (readValue & 0x8000)
        {
            isBusy = false;
        }
        else
        {
            isBusy = true;
        }

        if (readValue & 0x3200)
        {
            status = kStatus_Fail;
            break;
        }

    } while (isBusy);

    return status;
}



static status_t flexspi_nor_flash_erase_sector(FLEXSPI_Type *base, uint32_t address)
{
    // PRINTF("XXXXXXXX flexspi_nor_flash_erase_sector @ 0x%p\r\n", address);

    status_t status;
    flexspi_transfer_t flashXfer;

    // Write enable
    status = flexspi_nor_write_enable(base, address);

    if (status != kStatus_Success)
    {
        return status;
    }

    flashXfer.deviceAddress = address;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 4;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR;
    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    status = flexspi_nor_wait_bus_busy(base);

    return status;
}



static status_t flexspi_nor_flash_page_program(FLEXSPI_Type *base, uint32_t address, const uint32_t *src, uint32_t dataLen)
{
    // PRINTF("XXXXXXXX flexspi_nor_flash_page_program 0x%p\r\n", address);

//    PRINTF("XXXXXXXX Page contents:\r\n");
//    for (int i = 0 ; i < FLASH_PAGE_SIZE / 4; i++)
//        PRINTF("%08X\r\n", src[i]);

    status_t status;
    flexspi_transfer_t flashXfer;

    // Write enable
    status = flexspi_nor_write_enable(base, address);

    if (status != kStatus_Success)
    {
        return status;
    }

    // Prepare page program command
    flashXfer.deviceAddress = address;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Write;
    flashXfer.SeqNumber = 2;
    flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM;
    flashXfer.data = (uint32_t *)src;
    flashXfer.dataSize = dataLen;//FLASH_PAGE_SIZE;
    status = FLEXSPI_TransferBlocking(base, &flashXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    status = flexspi_nor_wait_bus_busy(base);

    return status;
}



static void flexspi_nor_init()
{
    flexspi_config_t config;
    FLEXSPI_GetDefaultConfig(&config);
    config.ahbConfig.enableAHBPrefetch = true;
    config.enableSckBDiffOpt = true;
    config.rxSampleClock = kFLEXSPI_ReadSampleClkExternalInputFromDqsPad;
    config.enableCombination = true;
    FLEXSPI_Init(FLEXSPI, &config);

    // Configure flash settings according to serial flash feature.
    FLEXSPI_SetFlashConfig(FLEXSPI, &deviceconfig, kFLEXSPI_PortA1);

    // Update LUT table.
    FLEXSPI_UpdateLUT(FLEXSPI, 0, customLUT, CUSTOM_LUT_LENGTH);

    // Do software reset.
    FLEXSPI_SoftwareReset(FLEXSPI);
}

#endif

/******************************** VARIABLES ***********************************/

static Mutex_t FLASH_LOCK = 0;

/***************************** PRIVATE METHODS ********************************/

/* Write to flash */
static status_t writeDataToFlash(uint32_t address, uint8_t *data, int32_t dataLen)
{
    // Program page(s)
   // uint32_t i;
    status_t status = kStatus_Success;
    uint32_t pageWriteLen;
   // uint8_t *flashPtr = (uint8_t *)(FlexSPI_AMBA_BASE + address);

    while (dataLen > 0)
    {
        pageWriteLen = dataLen;
        if (pageWriteLen > FLASH_PAGE_SIZE)
        {
            pageWriteLen = FLASH_PAGE_SIZE;
        }

        // Ensure we don't wrap around within current page at start of programming
        if (((address & (FLASH_PAGE_SIZE-1)) + pageWriteLen) & ~(FLASH_PAGE_SIZE-1))
        {
            pageWriteLen = FLASH_PAGE_SIZE - (address & (FLASH_PAGE_SIZE-1));
        }

        // Program data
        status = flexspi_nor_flash_page_program(FLEXSPI, address, (const uint32_t *)data, pageWriteLen);

        data += pageWriteLen;
        address += pageWriteLen;
        dataLen -= pageWriteLen;
    }

    // Do software reset to reset AHB buffer.
    FLEXSPI_SoftwareReset(FLEXSPI);

    return status;
}


/****************************** PUBLIC METHODS ********************************/
/*
 * \brief Initialises spi flash interface
 *
 * \param none
 * \return none
 */
void SPI_Flash_Init(void)
{
    status_t status;

    if (FLASH_LOCK)
    {
         //LOG_Info("FLASH already initialised");
         return;
    }

    if ((status = OSAL_InitMutex(&FLASH_LOCK, NULL)) != 0)
    {
        LOG_Error("Call to OSAL_InitMutex() failed, returned %d, Device is halting !!!!", status);
        while(1);
    }

    flexspi_nor_init();
}



/*
 * \brief Read data from spi flash
 *
 * \param address - flash location from which to read
 *        data    - buffer address for data
 *        dataLen - length of data to read
 * \return status - result
 */
int32_t SPI_Flash_Read(uint32_t address, uint8_t *data, uint32_t dataLen)
{
    if (!FLASH_LOCK)
    {
        SPI_Flash_Init();
    }

    OSAL_LockMutex(&FLASH_LOCK);

    // Read data
    memcpy(data, (void*)(FlexSPI_AMBA_BASE + address), dataLen);

    OSAL_UnLockMutex(&FLASH_LOCK);

    return 0;
}



/*
 * \brief Write data from spi flash
 *
 * \param address - flash location to write
 *        data    - buffer address of data
 *        dataLen - length of data to write
 * \return status - result
 */
int32_t SPI_Flash_Write(uint32_t address, uint8_t *data, uint32_t dataLen)
{
    int32_t status = -1;

    if (!FLASH_LOCK)
    {
        SPI_Flash_Init();
    }

    OSAL_LockMutex(&FLASH_LOCK);

    if ( writeDataToFlash(address, data, dataLen) == kStatus_Success )
    {
        status = 0;
    }
    else
    {
        LOG_Error("write data to flash failed, address 0x%08x",address);
    }

    OSAL_UnLockMutex(&FLASH_LOCK);

    return status;
}



/*
 * \brief Write data from spi flash
 *
 * \param address - flash sector to erase
 * \return status - result
 */
int32_t SPI_Flash_Erase(uint32_t address)
{
    int32_t status = -1;

    if (address & (FLASH_SECTOR_SIZE-1))
    {
        LOG_Warning("Erase sector address not on %dK sector boundary, ignoring request", FLASH_SECTOR_SIZE/0x1000);
        return status;
    }

    if (!FLASH_LOCK)
    {
        SPI_Flash_Init();
    }

    OSAL_LockMutex(&FLASH_LOCK);

    status = flexspi_nor_flash_erase_sector(FLEXSPI, address);

    OSAL_UnLockMutex(&FLASH_LOCK);

    return status;
}
