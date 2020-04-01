/*!****************************************************************************
 *
 * \file StoFileSystem.c
 *
 * \author Murat Cakmak
 *
 * \brief Ultra Primitive File System for Ameba Internal Flash.
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 *  Unauthorized copying of this file, via any medium is strictly prohibited
 *
 ******************************************************************************/

/****************************** INCLUDES **************************************/
#include "LOG_Api.h"
#include "OSAL_Api.h"
#include "SPI_Flash.h"
#include "STO_FileSystem.h"
#include <SPI_Flash.issi_32mbit.h>


/******************************** CONSTANTS ***********************************/

#define STO_FILE_BLOCK_SIZE                  FLASH_SECTOR_SIZE


// Max file name length
#define STO_MAX_FILE_COUNT                   2
#define STO_MAX_FILE_NAME_LEN                64

#define STO_MAX_FILE_SIZE                    STO_FLASH_SIZE / STO_MAX_FILE_COUNT

#define STO_FILE_HEADER_LEN                  96
#define STO_MAX_FILE_CONTENT_LEN             STO_MAX_FILE_SIZE - STO_FILE_HEADER_LEN

// Allowed open max file count at same time
#define STO_ALLOWED_MAX_OPEN_FILE_COUNT      2

#define INVALID_BLOCK_ADDRESS                0xFFFFFFFF



/*************************** TYPE DEFINITIONS *********************************/

/* File Header */
typedef struct
{
    /* File Name */
    uint8_t name[STO_MAX_FILE_NAME_LEN];
    /* File Size */
    uint32_t fileSize;
    /* Keep reserved to compliant header size with 96 bytes*/
    uint8_t __reserved[28];
} STOFileHeader;

/* All file information including content */
typedef struct
{
    STOFileHeader fileHeader;
    uint8_t content[STO_MAX_FILE_CONTENT_LEN];
} STOFile;

/* Open file attributes */
typedef struct
{
    bool inUse;
    uint32_t fileOffset;
    uint32_t readOffset;
    OSAL_AccessMode_e accessMode;
} OpenSTOFile;



/******************************** VARIABLES ***********************************/

static OpenSTOFile openFiles[STO_ALLOWED_MAX_OPEN_FILE_COUNT];
static Mutex_t STO_LOCK = NULL;



/***************************** PRIVATE METHODS *******************************/


/* Updates the first block of the file */
static int32_t updateFile(uint32_t fileAddress, STOFile* newContent)
{
    int32_t status = -1;

    /* We need to delete block first to apply changes */
    if ((status = SPI_Flash_Erase(fileAddress)) == 0)
    {
        /* Write new content to flash */
        status = SPI_Flash_Write(fileAddress, (uint8_t*)newContent, STO_FILE_BLOCK_SIZE);
    }
    else
    {
        LOG_Info("%s: Failed to erase flash at address 0x%08x", __func__, fileAddress);
    }

    return status;
}



/* Returns empty slot number in OpenFile container */
static int32_t getEmptySlot(void)
{
    for (int i = 0; i < STO_ALLOWED_MAX_OPEN_FILE_COUNT; i++)
    {
        if (openFiles[i].inUse == false)
        {
            return i;
        }
    }

    return -1;
}



/* Returns file address in flash */
static uint32_t getFileAddr(const char* fileName)
{
    STOFileHeader fileHeader;
    uint32_t fileAddr;

    for (int i = 0; i < STO_MAX_FILE_COUNT; i++)
    {
        /* Test next file */
        fileAddr = STO_FLASH_START + (i * STO_MAX_FILE_SIZE);

        /* Read just file header */
        SPI_Flash_Read(fileAddr, (uint8_t*)&fileHeader, sizeof(fileHeader));

        if (strcmp(fileHeader.name, fileName) == 0)
        {
            /* We found the file */
            return fileAddr;
        }
    }

    /* We could not find file in flash */
    return INVALID_BLOCK_ADDRESS;
}



/* Returns empty block in Flash for a new file */
static uint32_t findEmptyBlock(void)
{
    STOFileHeader fileHeader;
    uint32_t fileAddr;

    for (int i = 0; i < STO_MAX_FILE_COUNT; i++)
    {
        /* Test next block */
        fileAddr = STO_FLASH_START + (i * STO_MAX_FILE_SIZE);
        /* Read just file header */
        SPI_Flash_Read(fileAddr, (uint8_t *)&fileHeader, sizeof(fileHeader));

        /* If file size is not set before it is an empty block */
        if (fileHeader.fileSize == 0xFFFFFFFF)
        {
            return fileAddr;
        }
    }

    /* We could not find an empty block in flash */
    return INVALID_BLOCK_ADDRESS;
}



/****************************** PUBLIC METHODS ********************************/
/*
 * \brief Initialises Enso Storage File System Driver Module
 *
 * \param none
 * \return none
 */
void STO_Driver_Init(void)
{
    if (STO_LOCK)
    {
         LOG_Info("STO already initialised");
         return;
    }

    OSAL_InitMutex(&STO_LOCK, NULL);

    return;
}



/*
 * \brief Opens a file
 *
 * \param fileName File Name (Path)
 * \param accessMode Access mode for opened file
 *
 * \return handle to file. NULL in case of fail.
 *
 */
Handle_t STO_Open(const char* fileName, OSAL_AccessMode_e accessMode)
{
    if (strlen(fileName) >= STO_MAX_FILE_NAME_LEN)
    {
        LOG_Error("Store Name length (%d) is longer than allowed", strlen(fileName));
        return NULL;
    }

    OSAL_LockMutex(&STO_LOCK);

    /* Get file address in flash */
    uint32_t fileAddr = getFileAddr(fileName);

    if ((fileAddr == INVALID_BLOCK_ADDRESS) && (accessMode == READ_ONLY))
    {
        LOG_Error("File %s does not exist to read!", fileName);
        OSAL_UnLockMutex(&STO_LOCK);
        return NULL;
    }

    /* Get empty slot from open files container */
    int32_t emptySlot = getEmptySlot();

    if (emptySlot < 0)
    {
        LOG_Error("No empty slot for new file!");
        OSAL_UnLockMutex(&STO_LOCK);
        return NULL;
    }

    OpenSTOFile* openFile = &openFiles[emptySlot];
    openFile->accessMode = accessMode;
    openFile->readOffset = 0;

    /* It is a new file */
    if (fileAddr == INVALID_BLOCK_ADDRESS)
    {
        /* Find an empty block in flash for new file */
        uint32_t emptyBlockAddr = findEmptyBlock();

        if (INVALID_BLOCK_ADDRESS == emptyBlockAddr)
        {
            LOG_Error("Not enough place to create a new file");
            OSAL_UnLockMutex(&STO_LOCK);
            return NULL;
        }

        /* Prepare new file */
        static uint8_t newFile[FLASH_SECTOR_SIZE];
        STOFile *newfilePtr = (STOFile *)newFile;
        memset(newFile, 0xFF, sizeof(newFile));
        strcpy(newfilePtr->fileHeader.name, fileName);
        newfilePtr->fileHeader.fileSize = 0;

        /* Write new file to flash */
        SPI_Flash_Write(emptyBlockAddr, newFile, STO_FILE_BLOCK_SIZE);

        /* Keep File Flash Address */
        openFile->fileOffset = emptyBlockAddr;
    }
    else
    {
        /* Keep Flash Address of existing file */
        openFile->fileOffset = fileAddr;
    }

    /* Mark opened file as in use */
    openFile->inUse = true;

    OSAL_UnLockMutex(&STO_LOCK);

    return openFile;
}



/*
 * \brief Writes to file
 *
 * \param handle to be written file handle
 * \param buffer to be written data
 * \param buffer to be written data len
 *
 * \return 0 in success. -1 in case of fail.
 *
 */
int32_t STO_Write(Handle_t handle, const void* buffer, size_t nBytes)
{
    OpenSTOFile* openFile = (OpenSTOFile*)handle;

    if (openFile->accessMode == READ_ONLY)
    {
        LOG_Error("Invalid file access mode (READ_ONLY) to write file!");
        return -1;
    }

    OSAL_LockMutex(&STO_LOCK);

    static uint8_t updatedFile[FLASH_SECTOR_SIZE];
    STOFile *updatedFilePtr = (STOFile *)updatedFile;

    /* Read 1st block to RAM first */
    SPI_Flash_Read(openFile->fileOffset, updatedFile, sizeof(updatedFile));

    if (openFile->accessMode == WRITE_ONLY ||
        openFile->accessMode == READ_WRITE)
    {
        /* Write Modes overwrite content */
        updatedFilePtr->fileHeader.fileSize = 0;
        /* For sequential writes, lets change to access mode to append after first write */
        openFile->accessMode = WRITE_APPEND;
    }

    int32_t writeStart = updatedFilePtr->fileHeader.fileSize;
    int32_t remainingLen = STO_MAX_FILE_CONTENT_LEN - writeStart;
    int32_t writeLen = nBytes < remainingLen ? nBytes : remainingLen;

    /* Increase file size */
    updatedFilePtr->fileHeader.fileSize += writeLen;

    /* Update file 1st block in flash */
    updateFile(openFile->fileOffset, updatedFilePtr);

    /* Copy new content to file */
    SPI_Flash_Write(openFile->fileOffset + offsetof(STOFile,content) + writeStart, (uint8_t *)buffer, writeLen);

    OSAL_UnLockMutex(&STO_LOCK);

    return writeLen;
}



/*
 * \brief Reads from file
 *
 * \param handle to be read file handle
 * \param buffer to be read data
 * \param buffer to be read data len
 *
 * \return 0 in success. -1 in case of fail.
 *
 */
int32_t STO_Read(Handle_t handle, void* buffer, size_t nBytes)
{
    OpenSTOFile* openFile = (OpenSTOFile*)handle;

    if (openFile->accessMode == WRITE_ONLY)
    {
        LOG_Error("Invalid file access mode (WRITE_ONLY) to read file!");
        return -1;
    }

    OSAL_LockMutex(&STO_LOCK);

    STOFileHeader fileHeader;

    /* Read header to RAM */
    SPI_Flash_Read(openFile->fileOffset, (uint8_t *)&fileHeader, sizeof(fileHeader));

    int32_t remainingLen = fileHeader.fileSize - openFile->readOffset;
    int32_t readLen = nBytes < remainingLen ? nBytes : remainingLen;

    /* Read data to user buffer */
    SPI_Flash_Read(openFile->fileOffset + offsetof(STOFile,content) + openFile->readOffset, (uint8_t *)buffer, readLen);

    /* Increase Read offset for sequential reads */
    openFile->readOffset += readLen;

    OSAL_UnLockMutex(&STO_LOCK);

    return readLen;
}



/*
 * \brief Closes a file
 *
 * \param handle to be closed file handle
 *
 * \return 0 in success. -1 in case of fail.
 *
 */
int32_t STO_Close(Handle_t handle)
{
    OSAL_LockMutex(&STO_LOCK);

    OpenSTOFile* openFile = (OpenSTOFile*)handle;

    /* Just clean OpenFile objects. It also clears inUse flag to release file. */
    memset(openFile, 0, sizeof(OpenSTOFile));

    OSAL_UnLockMutex(&STO_LOCK);

    return 0;
}



/*
 * \brief Renames a file
 *
 * \param oldName to be renamed file Name
 * \param newName new file Name
 *
 * \return 0 in success. -1 in case of fail.
 *
 */
int32_t STO_Rename(const char * oldName, const char * newName)
{
    int32_t status = -1;

    if (strlen(newName) >= STO_MAX_FILE_NAME_LEN)
    {
        LOG_Error("Store Name length (%d) is longer than allowed", strlen(newName));
        return status;
    }

    OSAL_LockMutex(&STO_LOCK);
    /* Get file address in flash */
    uint32_t fileAddr = getFileAddr(oldName);

    if (fileAddr == INVALID_BLOCK_ADDRESS)
    {
        LOG_Error("File %s does not exist!", oldName);
        OSAL_UnLockMutex(&STO_LOCK);
        return status;
    }

    //////// [RE:added] Check if new name already exists
    uint32_t fileAddrExists = getFileAddr(newName);
    if (fileAddrExists != INVALID_BLOCK_ADDRESS)
    {
        LOG_Error("Filename already exists!");
        OSAL_UnLockMutex(&STO_LOCK);
        return status;
    }

    static uint8_t renamedFile[FLASH_SECTOR_SIZE];
    STOFile *renamedFilePtr = (STOFile *)renamedFile;

    /* Read flash content to RAM */
    if ( (status = SPI_Flash_Read(fileAddr, renamedFile, sizeof(renamedFile))) == 0 )
    {
        /* Change file name */
        strcpy(renamedFilePtr->fileHeader.name, newName);

        /* Update file in flash  */
        if ( (status = updateFile(fileAddr, renamedFilePtr)) != 0 )
        {
            LOG_Error("Failed to rename file %s to %s",oldName,newName);
        }
    }
    else
    {
        LOG_Error("Failed to read file %s",oldName);
    }

    OSAL_UnLockMutex(&STO_LOCK);

    return 0;
}



/*
 * \brief Removes(Deletes) a file
 *
 * \param fileName to be deleted file Name
 *
 * \return 0 in success. -1 in case of fail.
 *
 */
int32_t STO_Remove(const char * fileName)
{
    int32_t status = 0;

    OSAL_LockMutex(&STO_LOCK);

    /* Get file address in flash  */
    uint32_t fileAddr = getFileAddr(fileName);

    if (fileAddr == INVALID_BLOCK_ADDRESS)
    {
        LOG_Error("File %s does not exist!", fileName);
        OSAL_UnLockMutex(&STO_LOCK);
        return -1;
    }

    /* Erase file */
    for (int i = 0; i < STO_MAX_FILE_SIZE / STO_FILE_BLOCK_SIZE; i++)
    {
        if (SPI_Flash_Erase(fileAddr + (i * STO_FILE_BLOCK_SIZE)) != 0)
        {
            LOG_Error("Failed to erase 0x%08x", i * STO_FILE_BLOCK_SIZE);
            status = -1;
        }
    }

    OSAL_UnLockMutex(&STO_LOCK);

    return status;
}



/*
 * \brief Returns file size
 *
 * \param fileName File Name
 *
 * \return file size
 *
 */
int32_t STO_FileSize(const char * fileName)
{
    int32_t status = -1;

    OSAL_LockMutex(&STO_LOCK);

    /* Get file address in flash */
    uint32_t fileAddr = getFileAddr(fileName);

    if (fileAddr == INVALID_BLOCK_ADDRESS)
    {
        LOG_Error("File %s does not exist!", fileName);
        OSAL_UnLockMutex(&STO_LOCK);
        return 0;
    }

    STOFileHeader fileHeader;

    /* Just read flash header */
    if ( (status = SPI_Flash_Read(fileAddr, (uint8_t *)&fileHeader, sizeof(STOFileHeader))) != 0 )
    {
        LOG_Error("Failed to read file %s", fileName);
        OSAL_UnLockMutex(&STO_LOCK);
        return 0;
    }

    OSAL_UnLockMutex(&STO_LOCK);

    return fileHeader.fileSize;
}



/*
 * \brief Returns whether file exists
 *
 * \param fileName File Name
 *
 * \return true if file exits
 *
 */
bool STO_FileExist(const char * fileName)
{
    bool status = false;

    OSAL_LockMutex(&STO_LOCK);

    if (getFileAddr(fileName) != INVALID_BLOCK_ADDRESS)
    {
        status = true;
    }

    OSAL_UnLockMutex(&STO_LOCK);

    return status;
}



/*
 * \brief Formats all flash storage.
 *
 * \param none
 *
 * \return none
 */
void STO_FormatDevice(void)
{
    OSAL_LockMutex(&STO_LOCK);

    for (int i = 0; i < STO_FLASH_SIZE / FLASH_SECTOR_SIZE; i++)
    {
        if (SPI_Flash_Erase(STO_FLASH_START + (i * FLASH_SECTOR_SIZE)) != 0)
        {
            LOG_Error("Failed to erase 0x%08x", i * FLASH_SECTOR_SIZE);
        }
    }

    OSAL_UnLockMutex(&STO_LOCK);
}



/*
 * \brief Lists all files in flash storage.
 *
 * \param none
 *
 * \return none
 */
void STO_List(void)
{
    OSAL_LockMutex(&STO_LOCK);

    STOFileHeader fileHeader;
    uint32_t fileAddr;

    for (int i = 0; i < STO_MAX_FILE_COUNT; i++)
    {
        /* Test next file */
        fileAddr = STO_FLASH_START + (i * STO_MAX_FILE_SIZE);
        /* Read just file header */
        SPI_Flash_Read(fileAddr, (uint8_t *)(&fileHeader), sizeof(fileHeader));

        LOG_Info("@%p %d of %d : %s", fileAddr, (i + 1), STO_MAX_FILE_COUNT, fileHeader.name);
    }

    OSAL_UnLockMutex(&STO_LOCK);
}
