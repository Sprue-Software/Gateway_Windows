/*!****************************************************************************
 *
 * \file FileSystem.c
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
#include "EFS_FileSystem.h"



/******************************** CONSTANTS ***********************************/

#define FS_FILE_BLOCK_SIZE                  FLASH_SECTOR_SIZE

// Max file name length
#define FS_MAX_FILE_COUNT                   (FS_FLASH_SIZE / FS_FILE_BLOCK_SIZE)    //////// 1-file-per-sector
#define FS_MAX_FILE_NAME_LEN                64

// To simplify implementation lets keeps file content size up to 4000
//////// (4000 + 96 for the header = one 4096 sector)
#define FS_MAX_FILE_CONTENT_LEN             4000    //////// 1-file-per-sector (4000 + 96 for the header = one 4096 sector)
#define FS_FILE_HEADER_LEN                  96      //////// 1-file-per-sector (4000 + 96 for the header = one 4096 sector)

// Allowed open max file count at same time
#define FS_ALLOWED_MAX_OPEN_FILE_COUNT      8

#define INVALID_BLOCK_ADDRESS               0xFFFFFFFF



/*************************** TYPE DEFINITIONS *********************************/

/* File Header */
typedef struct
{
    /* File Name */
    uint8_t name[FS_MAX_FILE_NAME_LEN];
    /* File Size */
    uint32_t fileSize;
    /* Keep reserved to compliant header size with 96 bytes*/
    uint8_t __reserved[28];
} EFSFileHeader;

/* All file information including content */
typedef struct
{
    EFSFileHeader fileHeader;
    uint8_t content[FS_MAX_FILE_CONTENT_LEN];
} EFSFile;

/* Open file attributes */
typedef struct
{
    bool inUse;
    uint32_t fileOffset;
    uint32_t readOffset;
    OSAL_AccessMode_e accessMode;
} OpenEFSFile;



/******************************** VARIABLES ***********************************/

static OpenEFSFile openFiles[FS_ALLOWED_MAX_OPEN_FILE_COUNT];
static Mutex_t EFS_LOCK = NULL;



/***************************** PRIVATE METHODS *******************************/


/* Updates a file */
static int32_t updateFile(uint32_t fileAddress, EFSFile* newContent)
{
    int32_t status = -1;

    /* We need to delete block first to apply changes */
    if ((status = SPI_Flash_Erase(fileAddress)) == 0)
    {
        /* Write new content to flash */
        status = SPI_Flash_Write(fileAddress, (uint8_t*)newContent, FS_FILE_BLOCK_SIZE);
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
    for (int i = 0; i < FS_ALLOWED_MAX_OPEN_FILE_COUNT; i++)
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
    EFSFileHeader fileHeader;
    uint32_t fileAddr;

    for (int i = 0; i < FS_MAX_FILE_COUNT; i++)
    {
        /* Test next file */
        fileAddr = FS_FLASH_START + (i * FLASH_SECTOR_SIZE);

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
    EFSFileHeader fileHeader;
    uint32_t fileAddr;

    for (int i = 0; i < FS_MAX_FILE_COUNT; i++)
    {
        /* Test next block */
        fileAddr = FS_FLASH_START + (i * FLASH_SECTOR_SIZE);
        /* Read just file header */
        SPI_Flash_Read(fileAddr, (uint8_t *)&fileHeader, sizeof(fileHeader));

        /* If file size is not set before it is an empty block */
        if (fileHeader.fileSize == 0xFFFFFFFF)
        {
            return fileAddr;
        }
    }

    /* We could not find any empty block in flash */
    return INVALID_BLOCK_ADDRESS;
}



/****************************** PUBLIC METHODS ********************************/
/*
 * \brief Initialises Enso File System Module
 *
 * \param none
 * \return none
 */
void EFS_Init(void)
{
    if (EFS_LOCK)
    {
         LOG_Info("EFS already initialised");
         return;
    }

    /* We cannot check in Precompile time so lets check here. */
    if (sizeof(EFSFile) != FS_FILE_BLOCK_SIZE)
    {
        LOG_Error("Invalid File Structure! Device is halting!");
        while (1);
    }

    OSAL_InitMutex(&EFS_LOCK, NULL);
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
Handle_t EFS_Open(const char* fileName, OSAL_AccessMode_e accessMode)
{
    if (strlen(fileName) >= FS_MAX_FILE_NAME_LEN)
    {
        LOG_Error("Store Name length (%d) is longer than allowed", strlen(fileName));
        return NULL;
    }

    OSAL_LockMutex(&EFS_LOCK);

    /* Get file address in flash */
    uint32_t fileAddr = getFileAddr(fileName);

    if ((fileAddr == INVALID_BLOCK_ADDRESS) && (accessMode == READ_ONLY))
    {
        LOG_Error("File %s does not exist to read!", fileName);
        OSAL_UnLockMutex(&EFS_LOCK);
        return NULL;
    }

    /* Get empty slot from open files container */
    int32_t emptySlot = getEmptySlot();

    if (emptySlot < 0)
    {
        LOG_Error("No empty slot for new file!");
        OSAL_UnLockMutex(&EFS_LOCK);
        return NULL;
    }

    OpenEFSFile* openFile = &openFiles[emptySlot];
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
            OSAL_UnLockMutex(&EFS_LOCK);
            return NULL;
        }

        /* Prepare new file */
        static EFSFile newFile = { 0 };
        memset(&newFile, 0xFF, sizeof(newFile));
        strcpy(newFile.fileHeader.name, fileName);
        newFile.fileHeader.fileSize = 0;

        /* Write new file to flash */
        SPI_Flash_Write(emptyBlockAddr, (uint8_t*)&newFile, FS_FILE_BLOCK_SIZE);

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

    OSAL_UnLockMutex(&EFS_LOCK);

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
int32_t EFS_Write(Handle_t handle, const void* buffer, size_t nBytes)
{
    OpenEFSFile* openFile = (OpenEFSFile*)handle;

    if (openFile->accessMode == READ_ONLY)
    {
        LOG_Error("Invalid file access mode (READ_ONLY) to write file!");
        return -1;
    }

    OSAL_LockMutex(&EFS_LOCK);

    static EFSFile updatedFile = { 0 };

    /* Read content to RAM first */
    SPI_Flash_Read(openFile->fileOffset, (uint8_t*)&updatedFile, sizeof(updatedFile));

    if (openFile->accessMode == WRITE_ONLY ||
        openFile->accessMode == READ_WRITE)
    {
        /* Write Modes overwrite content */
        updatedFile.fileHeader.fileSize = 0;
        /* For sequential writes, lets change to access mode to append after first write */
        openFile->accessMode = WRITE_APPEND;
    }

    int32_t remainingLen = FS_MAX_FILE_CONTENT_LEN - updatedFile.fileHeader.fileSize;
    int32_t writeLen = nBytes < remainingLen ? nBytes : remainingLen;

    /* Copy new content to file */
    memcpy(&updatedFile.content[updatedFile.fileHeader.fileSize], buffer, writeLen);

    /* Increase file size */
    updatedFile.fileHeader.fileSize += writeLen;

    /* Update file in flash */
    updateFile(openFile->fileOffset, &updatedFile);

    OSAL_UnLockMutex(&EFS_LOCK);

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
int32_t EFS_Read(Handle_t handle, void* buffer, size_t nBytes)
{
    OpenEFSFile* openFile = (OpenEFSFile*)handle;

    if (openFile->accessMode == WRITE_ONLY)
    {
        LOG_Error("Invalid file access mode (WRITE_ONLY) to read file!");
        return -1;
    }

    OSAL_LockMutex(&EFS_LOCK);

    static EFSFile readFile;

    /* Read content to RAM */
    SPI_Flash_Read(openFile->fileOffset, (uint8_t *)&readFile, sizeof(readFile));

    int32_t remainingLen = readFile.fileHeader.fileSize - openFile->readOffset;
    int32_t readLen = nBytes < remainingLen ? nBytes : remainingLen;

    /*Copy to user buffer */
    memcpy(buffer, &readFile.content[openFile->readOffset], readLen);

    /* Increase Read offset for sequential reads */
    openFile->readOffset += readLen;

    OSAL_UnLockMutex(&EFS_LOCK);

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
int32_t EFS_Close(Handle_t handle)
{
    OSAL_LockMutex(&EFS_LOCK);

    OpenEFSFile* openFile = (OpenEFSFile*)handle;

    /* Just clean OpenFile objects. It also clears inUse flag to release file. */
    memset(openFile, 0, sizeof(OpenEFSFile));

    OSAL_UnLockMutex(&EFS_LOCK);

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
int32_t EFS_Rename(const char * oldName, const char * newName)
{
    int32_t status = -1;

    if (strlen(newName) >= FS_MAX_FILE_NAME_LEN)
    {
        LOG_Error("Store Name length (%d) is longer than allowed", strlen(newName));
        return status;
    }

    OSAL_LockMutex(&EFS_LOCK);
    /* Get file address in flash */
    uint32_t fileAddr = getFileAddr(oldName);

    if (fileAddr == INVALID_BLOCK_ADDRESS)
    {
        LOG_Error("File %s does not exist!", oldName);
        OSAL_UnLockMutex(&EFS_LOCK);
        return status;
    }

    //////// [RE:added] Check if new name already exists
    uint32_t fileAddrExists = getFileAddr(newName);
    if (fileAddrExists != INVALID_BLOCK_ADDRESS)
    {
        LOG_Error("Filename already exists!");
        OSAL_UnLockMutex(&EFS_LOCK);
        return status;
    }

    static EFSFile renamedFile = { 0 };

    /* Read flash content to RAM */
    if ( (status = SPI_Flash_Read(fileAddr, (uint8_t *)&renamedFile, sizeof(renamedFile))) == 0 )
    {
        /* Change file name */
        strcpy(renamedFile.fileHeader.name, newName);

        /* Update file in flash  */
        if ( (status = updateFile(fileAddr, &renamedFile)) != 0 )
        {
            LOG_Error("Failed to rename file %s to %s",oldName,newName);
        }
    }
    else
    {
        LOG_Error("Failed to read file %s",oldName);
    }

    OSAL_UnLockMutex(&EFS_LOCK);

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
int32_t EFS_Remove(const char * fileName)
{
    int32_t status = -1;

    OSAL_LockMutex(&EFS_LOCK);

    /* Get file address in flash  */
    uint32_t fileAddr = getFileAddr(fileName);

    if (fileAddr == INVALID_BLOCK_ADDRESS)
    {
        LOG_Error("File %s does not exist!", fileName);
        OSAL_UnLockMutex(&EFS_LOCK);
        return -1;
    }

    /* Erase file */
    if ( (status = SPI_Flash_Erase(fileAddr)) != 0 )
    {
        LOG_Error("Failed to remove file %s", fileName);
    }

    OSAL_UnLockMutex(&EFS_LOCK);

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
int32_t EFS_FileSize(const char * fileName)
{
    int32_t status = -1;

    OSAL_LockMutex(&EFS_LOCK);

    /* Get file address in flash */
    uint32_t fileAddr = getFileAddr(fileName);

    if (fileAddr == INVALID_BLOCK_ADDRESS)
    {
        LOG_Error("File %s does not exist!", fileName);
        OSAL_UnLockMutex(&EFS_LOCK);
        return 0;
    }

    EFSFileHeader fileHeader;

    /* Just read flash header */
    if ( (status = SPI_Flash_Read(fileAddr, (uint8_t *)&fileHeader, sizeof(EFSFileHeader))) != 0 )
    {
        LOG_Error("Failed to read file %s", fileName);
        OSAL_UnLockMutex(&EFS_LOCK);
        return 0;
    }

    OSAL_UnLockMutex(&EFS_LOCK);

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
bool EFS_FileExist(const char * fileName)
{
    bool status = false;

    OSAL_LockMutex(&EFS_LOCK);

    if (getFileAddr(fileName) != INVALID_BLOCK_ADDRESS)
    {
        status = true;
    }

    OSAL_UnLockMutex(&EFS_LOCK);

    return status;
}



/*
 * \brief Formats all flash storage.
 *
 * \param none
 *
 * \return none
 */
void EFS_FormatDevice(void)
{
    OSAL_LockMutex(&EFS_LOCK);

    for (int i = 0; i < FS_MAX_FILE_COUNT; i++)
    {
        if (SPI_Flash_Erase(FS_FLASH_START + (i * FLASH_SECTOR_SIZE)) != 0)
        {
            LOG_Error("Failed to erase 0x%08x", i * FLASH_SECTOR_SIZE);
        }
    }

    OSAL_UnLockMutex(&EFS_LOCK);
}



/*
 * \brief Lists all files in flash storage.
 *
 * \param none
 *
 * \return none
 */
void EFS_List(void)
{
    OSAL_LockMutex(&EFS_LOCK);

    EFSFileHeader fileHeader;
    uint32_t fileAddr;

    for (int i = 0; i < FS_MAX_FILE_COUNT; i++)
    {
        /* Test next file */
        fileAddr = FS_FLASH_START + (i * FLASH_SECTOR_SIZE);
        /* Read just file header */
        SPI_Flash_Read(fileAddr, (uint8_t *)(&fileHeader), sizeof(fileHeader));

        LOG_Info("@%p %d of %d : %s", fileAddr, (i + 1), FS_MAX_FILE_COUNT, fileHeader.name);
    }

    OSAL_UnLockMutex(&EFS_LOCK);
}
