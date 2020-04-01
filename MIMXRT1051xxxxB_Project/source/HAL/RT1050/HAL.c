/*!****************************************************************************
 *
 * \file HAL.c
 *
 * \author Evelyne Donnaes
 *
 * \brief Hardware Abstraction Layer Implementation
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "HAL.h"
#include "OSAL_Api.h"
#include "LOG_Api.h"
#include "SPI_Flash.h"

#include "board.h"
#include "LED_manager.h"

 static uint8_t serialNumstr[250];
 extern struct netif fsl_netif0; //Change By nishi for DHCP Issue
 extern struct netif fsl_netif1; // Change By Nishi  For DHCP Issue
extern void dump_net_config(struct netif *intf);

// static uint8_t serial[SERIAL_NUM_FLASH_SIZE];
/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

/*!****************************************************************************
 * Constants
 *****************************************************************************/

/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/



/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * \name HAL_Initialise
 *
 * \brief Initialise the HAL - all leds off.
 */
void HAL_Initialise(void)
{
    for (int led = 0; led < HAL_NO_LEDS; led++)
    {
        LED_Manager_EnsoAgent_SetLed(led, HAL_LEDSTATE_OFF);
    }
}

/**
 * \name HAL_RebootDevice
 *
 * \brief Reboots
 */
void HAL_Reboot(void)
{
    NVIC_SystemReset();
}

/**
 * \name   HAL_SetLed
 *
 * \brief  Turn the selected LED on or off
 *
 * \param  led   - The LED number
 *
 * \param  value - 0 = off, 1 = 0n
 *
 * \return 0 = status, -1 = error
 */
int16_t HAL_SetLed(uint8_t led, uint8_t value)
{
    if (led >= HAL_NO_LEDS)
    {
        LOG_Error("Invalid LED %u : value %u", led, value);
        return -1;
    }
    if (value != HAL_LEDSTATE_OFF && value != HAL_LEDSTATE_ON)
    {
        LOG_Error("LED %u : Invalid value %u", led, value);
        return -1;
    }
    /* There is no LED when running locally so just set led_state and print a message.
     */
    LOG_Info("LED %u %s", led, value == HAL_LEDSTATE_ON ? "On" : "Off");

    // Switch LED on board
    LED_Manager_EnsoAgent_SetLed(led, value);
    return 0;
}

/**
 * \name   HAL_GetLed
 *
 * \brief  Get the state of a LED
 *
 * \param  led   - The LED number
 *
 * \param  value - 0 = off, 1 = 0n
 *
 * \return 0 = status, -1 = error
 */
int16_t HAL_GetLed(uint8_t led, uint8_t *value)
{
    int16_t ret = 0;

    if (led >= HAL_NO_LEDS)
    {
        return -1;
    }

    if (value == NULL)
    {
        return -1;
    }

    LED_Manager_EnsoAgent_GetLed(led,value);
    LOG_Info("LED %u state is %u", led, *value);

    return ret;
}

/**
 * \name   HAL_GetMACAddressString
 *
 * \brief  Get the local MAC address
 *         Mac address is passed in the environment variable MACADDRESS
 *
 * \return Null-terminated representation of the local MAC address or NULL
 *         if no MAC address is available
 */
const char *    HAL_GetMACAddressString(void)
{
    static char mac_str[12 + 1];

    if (mac_str[0] == '\0')
    {
#ifdef FORCE_MAC_ADDRESS
        uint8_t mac[MAC_ADDR_FLASH_SIZE] = FORCE_MAC_ADDRESS;
#else 
        uint8_t mac[MAC_ADDR_FLASH_SIZE];
        SPI_Flash_Read(MAC_ADDR_FLASH_START, mac, MAC_ADDR_FLASH_SIZE);   //////// [RE:NB] This assumes SPI Flash has been initialised!
#endif
        sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    LOG_Error(">>>>> MAC ADDRESS STRING = %s",mac_str);

    return mac_str;
}

/**
 * \name   HAL_GetMACAddress
 *
 * \brief  Get the local MAC address
 *         Mac address is passed in the environment variable MACADDRESS
 *
 * \return 64-bit integer representation of the local MAC address or 0
 *         if no MAC address is available
 */
const uint64_t  HAL_GetMACAddress(void)
{
#ifdef FORCE_MAC_ADDRESS
        uint8_t mac[MAC_ADDR_FLASH_SIZE] = FORCE_MAC_ADDRESS;
#else 
        uint8_t mac[MAC_ADDR_FLASH_SIZE];
        SPI_Flash_Read(MAC_ADDR_FLASH_START, mac, MAC_ADDR_FLASH_SIZE);   //////// [RE:NB] This assumes SPI Flash has been initialised!
#endif
        uint64_t macAddr = 0;
// This Fucntion is called BY Ensoagent frequently , So Added LOgic for DHCP and memory Check 
    macAddr = (((uint64_t)mac[0])<<40) |
              (((uint64_t)mac[1])<<32) |
              (((uint64_t)mac[2])<<24) |
              (((uint64_t)mac[3])<<16) |
              (((uint64_t)mac[4])<<8)  |
              (((uint64_t)mac[5]));

   LOG_Error(">>>>> MAC ADDRESS = 0x%02x: 0x%02x: 0x%02x: 0x%02x: 0x%02x: 0x%02x free memory= %d",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],xPortGetFreeHeapSize());
  // dump_net_config(&fsl_netif0);
    return macAddr;
}

//////// [RE:added]
/**
 * \name   HAL_GetMACAddressBytes
 *
 * \brief  Get the local MAC address
 *
 * \return byte representation of the local MAC address
 */
void HAL_GetMACAddressBytes(const uint8_t* mac)
{
#ifdef FORCE_MAC_ADDRESS
	uint8_t force_mac[MAC_ADDR_FLASH_SIZE] = FORCE_MAC_ADDRESS;
	memcpy((uint8_t *)mac,force_mac,sizeof(force_mac));
#else 
    SPI_Flash_Read(MAC_ADDR_FLASH_START, (uint8_t*)mac, MAC_ADDR_FLASH_SIZE);   //////// [RE:NB] This assumes SPI Flash has been initialised!
#endif

    LOG_Error(">>>>> MAC ADDRESS = 0x%02x: 0x%02x: 0x%02x: 0x%02x: 0x%02x: 0x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}


//////// [RE:added]
/**
 * \name   HAL_GetRootCA
 *
 * \brief  Get the rootCA value from flash
 *
 * \return ptr to buffer containing rootCA data
 */
uint8_t * HAL_GetRootCA(void)
{
    static uint8_t rootCaDataFlash[FLASH_SECTOR_SIZE];
    if (rootCaDataFlash[0] == 0)
    {
        SPI_Flash_Read(ROOTCA_FLASH_START, (uint8_t*)rootCaDataFlash, FLASH_SECTOR_SIZE);
    }
    return rootCaDataFlash;
}


//////// [RE:added]
/**
 * \name   HAL_GetCert
 *
 * \brief  Get the certificate value from flash
 *
 * \return ptr to buffer containing certificate data
 */
uint8_t * HAL_GetCertificate(void)
{
    static uint8_t certificateDataFlash[FLASH_SECTOR_SIZE];
    if (certificateDataFlash[0] == 0)
    {
        SPI_Flash_Read(CERT_FLASH_START, certificateDataFlash, FLASH_SECTOR_SIZE);
    }
    return certificateDataFlash;
}


//////// [RE:added]
//////// [RE:added]
/**
 * \name   HAL_GetKey
 *
 * \brief  Get the private key value from flash
 *
 * \return ptr to buffer containing key data
 */
uint8_t * HAL_GetKey(void)
{
    static uint8_t keyDataFlash[FLASH_SECTOR_SIZE];
    if (keyDataFlash[0] == 0)
    {
        SPI_Flash_Read(KEY_FLASH_START, keyDataFlash, FLASH_SECTOR_SIZE);
    }
    return keyDataFlash;
}


//////// [RE:added]
/**
 * \name   HAL_GetSerialNum
 *
 * \brief  Get the gateway serial number from flash
 *
 * \return ptr to null-terminated serial number string if serial number valid (24 bytes)
 *         null - invalid checksum
 */
uint8_t * HAL_GetSerialNumber(void)
{
    uint8_t* serialNum = NULL;
    static uint8_t serialNumFlash[SERIAL_NUM_FLASH_SIZE];
    uint8_t checksum = 0;

    SPI_Flash_Read(SERIAL_NUM_FLASH_START, serialNumFlash, sizeof(serialNumFlash));   //////// [RE:NB] This assumes SPI Flash has been initialised!

    // validate checksum
    for (int i=0; i<(sizeof(serialNumFlash)-1); i++)
    {
        checksum += serialNumFlash[i];
    }

    if (checksum == serialNumFlash[sizeof(serialNumFlash)-1])
    {
        // update return pointer to serial number
        serialNumFlash[sizeof(serialNumFlash)-1] = 0;
        serialNum = serialNumFlash;
        LOG_Info("Serial number = '%s'",serialNumFlash);
    }
    else
    {
        LOG_Error("Serial number invalid");
    }

    return serialNum;
}

void HAL_serialBytes(const uint8_t* mac)
{

    SPI_Flash_Read(SERIAL_NUM_FLASH_START, (uint8_t*)mac, SERIAL_NUM_FLASH_SIZE);   //////// [RE:NB] This assumes SPI Flash has been initialised!


    LOG_Error(">>>>> Serial data = 0x%02x: 0x%02x: 0x%02x: 0x%02x: 0x%02x: 0x%02x",mac[9],mac[10],mac[11],mac[12],mac[13],mac[14]);
}
//////// [RE:added]
/**
 * \name   HAL_GetSerialNumberString
 *
 * \brief  Get the gateway serial number from flash
 *
 * \return ptr to null-terminated serial number string if serial number valid (24 bytes)
 *         null - invalid checksum
 */

const char * HAL_GetSerialNumberString(void)
{
  static uint8_t serial[SERIAL_NUM_FLASH_SIZE];
  //static char serialNumstr[200];
  SPI_Flash_Read(SERIAL_NUM_FLASH_START, serial, SERIAL_NUM_FLASH_SIZE);   //////// [RE:NB] This assumes SPI Flash has been initialised!
   
    return serial;
}

//////// [RE:added]
/**
 * \name   HAL_GetWisafeCalibration
 *
 * \brief  Get the wisafe calibration value from flash
 *
 * \return ptr to wisafe calibration data (4 bytes)
 *         null - invalid checksum
 */
uint8_t * HAL_GetWisafeCalibration(void)
{
    uint8_t* wisafeCal = NULL;
    static uint8_t wisafeCalFlash[WISAFE_CAL_FLASH_SIZE];
    uint8_t checksum = 0;

    SPI_Flash_Read(WISAFE_CAL_FLASH_START, wisafeCalFlash, sizeof(wisafeCalFlash));   //////// [RE:NB] This assumes SPI Flash has been initialised!

    // validate checksum
    for (int i=0; i<(sizeof(wisafeCalFlash)-1); i++)
    {
        checksum += wisafeCalFlash[i];
    }

    if (checksum == wisafeCalFlash[sizeof(wisafeCalFlash)-1])
    {
        // update return pointer to calibration data
        wisafeCalFlash[sizeof(wisafeCalFlash)-1] = 0;
        wisafeCal = wisafeCalFlash;
        LOG_Info("Wisafe calibration data = 0x%02x 0x%02x 0x%02x 0x%02x",wisafeCal[0],wisafeCal[1],wisafeCal[2],wisafeCal[3]);
    }
    else
    {
        LOG_Error("Wisafe calibration data invalid");
    }

    return wisafeCal;
}


//////// [RE:added]
/**
 * \name   HAL_SetActiveImageFlag
 *
 * \brief  Set the active image flag to specify which image (A or B) the bootloader should run at start-up
 *
 * \return  0 - success
 *         -1 - failure
 */
int32_t HAL_SetActiveImageFlag(uint8_t activeImageFlag)
{
    if ((activeImageFlag == 0x0a) || (activeImageFlag == 0x0b))
    {
        if (SPI_Flash_Erase(FLASHMAP_IMAGE_FLAG_SECTOR) == 0)
        {
            return SPI_Flash_Write(FLASHMAP_IMAGE_FLAG_SECTOR, &activeImageFlag, 1);
        }
        else
        {
            LOG_Error("Failed to erase sector");
        }
    }
    else
    {
        LOG_Error("Invalid value 0x%02x. Valid values are 0x0a and 0x0b",activeImageFlag);
    }
    return -1;
}


//////// [RE:added]
/**
 * \name   HAL_GetActiveImageFlag
 *
 * \brief  Get the active image flag specifying which image (A or B) the bootloader should run at start-up
 *
 * \return  activeImageFlag
  */
uint8_t HAL_GetActiveImageFlag(void)
{
    uint8_t activeImageFlag;

    SPI_Flash_Read(FLASHMAP_IMAGE_FLAG_SECTOR, &activeImageFlag, sizeof(activeImageFlag));
    if ((activeImageFlag == 0x0a) || (activeImageFlag == 0x0b))
    {
        LOG_Error("Active image flag value 0x%02x read from flash",activeImageFlag);
    }
    else
    {
        LOG_Error("Read invalid active image flag value 0x%02x. Valid values are 0x0a and 0x0b",activeImageFlag);
    }

    return activeImageFlag;
}

#if FUNCTIONAL_TEST
//////// [RE:added]
/**
 * \name   HAL_SetParamFlashBytes
 *
 * \brief  Programs the specified data into the requested location within the flash param sector
 *
 * \param  offset  - offset of flash param block to be erased
 *         data    - new data to be written
 *         dataLen - size of data to be new data
 *
 * \return  0 - success
 *         -1 - failure
 */
static int32_t HAL_SetParamFlashBytes(uint32_t offset, uint8_t * data, uint32_t dataLen)
{
    static uint8_t flashData[FLASH_SECTOR_SIZE];
    uint8_t * paramFlash = (uint8_t *)PARAM_ADDR_FLASH_START;

    if (data == NULL)
    {
        LOG_Error("null param, ignoring request");
    }

    if ((offset+dataLen) > FLASH_SECTOR_SIZE)
    {
        LOG_Error("Specified area of flash (offset = %d, data length = %d) exceeds sector size (%d), ignoring request",offset,dataLen,FLASH_SECTOR_SIZE);
        return -1;
    }

    for (int i=0; i<dataLen; i++)
    {
        // check area of flash to be programmed is already set to 0xFF
        if (paramFlash[i] != 0xFF)
        {
            // read complete sector into ram
            SPI_Flash_Read(PARAM_ADDR_FLASH_START, flashData, sizeof(flashData));

            // set the specified area within the sector
            memcpy(&flashData[offset],data,dataLen);

            // erase sector
            if (SPI_Flash_Erase(PARAM_ADDR_FLASH_START) == 0)
            {
                 // write modified data to flash
                return SPI_Flash_Write(PARAM_ADDR_FLASH_START, flashData, sizeof(flashData));
            }
            else
            {
                LOG_Error("Failed to erase sector. Check that the device is not write protected");
                return -1;
            }
        }
    }

    // area of flash already erased to 0xFF so OK to write directly
    return SPI_Flash_Write(PARAM_ADDR_FLASH_START+offset, data, sizeof(dataLen));
}


//////// [RE:added]
/**
 * \name   HAL_SetMacAddress
 *
 * \brief  Programs the 6-byte mac address value into flash
 *         This assumes that the area of flash to be programmed is set to 0xFF
 *
 * \return  0 - success
 *         -1 - failure
 */
int32_t HAL_SetMacAddress(uint8_t * macAddrValue)
{
    return HAL_SetParamFlashBytes(MAC_ADDR_FLASH_START - PARAM_ADDR_FLASH_START, macAddrValue, MAC_ADDR_FLASH_SIZE);
}


//////// [RE:added]
/**
 * \name   HAL_SetSerialNumber
 *
 * \brief  Set the unique gateway serial number in flash
 *         This assumes that the area of flash to be programmed is set to 0xFF
 *
 * \return  0 - success
 *         -1 - failure
 */
int32_t HAL_SetSerialNumber(const uint8_t * serialNum)
{
    uint8_t serialNumFlash[SERIAL_NUM_FLASH_SIZE];
    uint8_t checksum = 0;

    // prepare data and add checksum
    for (int i=0; i<(sizeof(serialNumFlash)-1); i++)
    {
        serialNumFlash[i] = serialNum[i];
        checksum += serialNum[i];
    }
    serialNumFlash[sizeof(serialNumFlash)-1] = checksum;

    return HAL_SetParamFlashBytes(SERIAL_NUM_FLASH_START - PARAM_ADDR_FLASH_START, serialNumFlash, sizeof(serialNumFlash));
}


//////// [RE:added]
/**
 * \name   HAL_SetWisafeCalibration
 *
 * \brief  Set the wisafe calibration value in flash
 *         This assumes that the area of flash to be programmed is set to 0xFF
 *
 * \return  0 - success
 *         -1 - failure
 */
int32_t HAL_SetWisafeCalibration(const uint8_t * wisafeCalValue)
{
    uint8_t wisafeCalFlash[WISAFE_CAL_FLASH_SIZE];
    uint8_t checksum = 0;

    // prepare data and add checksum
    for (int i=0; i<(sizeof(wisafeCalFlash)-1); i++)
    {
        wisafeCalFlash[i] = wisafeCalValue[i];
        checksum += wisafeCalValue[i];
    }
    wisafeCalFlash[sizeof(wisafeCalFlash)-1] = checksum;

    return HAL_SetParamFlashBytes(WISAFE_CAL_FLASH_START - PARAM_ADDR_FLASH_START, wisafeCalFlash, sizeof(wisafeCalFlash));
}


//////// [RE:added]
/**
 * \name   HAL_SetRootCA
 *
 * \brief  Set the rootCA value in flash
 *
 * \param  rootCaValue - The rootCA value represented as a null-terminated ASCII string to be programmed into flash
 *
 * \return 0  - success
 *         -1 - failure
 */
int32_t HAL_SetRootCA(const uint8_t * rootCaValue)
{
    if (rootCaValue)
    {
        int len = strlen(rootCaValue) + 1;

        if (len <= ROOTCA_FLASH_SIZE)
        {
            if (SPI_Flash_Erase(ROOTCA_FLASH_START) == 0)
            {
                return SPI_Flash_Write(ROOTCA_FLASH_START, (uint8_t *)rootCaValue, len);
            }
            else
            {
                LOG_Error("Failed to erase sector. Check that the device is not write protected");
            }
        }
        else
        {
            LOG_Error("Provided rootCA value length (%d) exceeds the size of allocated storage (%d)",len,ROOTCA_FLASH_SIZE);
        }
    }
    else
    {
        LOG_Error("Invalid null param");
    }
    return -1;
}


//////// [RE:added]
/**
 * \name   HAL_SetCertificate
 *
 * \brief  Set the certificate value in flash
 *
 * \param  certificateValue - The certificate value represented as a null-terminated ASCII string to be programmed into flash
 *
 * \return 0  - success
 *         -1 - failure
 */
int32_t HAL_SetCertificate(const uint8_t * certificateValue)
{
    if (certificateValue)
    {
        int len = strlen(certificateValue) + 1;

        if (len <= CERT_FLASH_SIZE)
        {
            if (SPI_Flash_Erase(CERT_FLASH_START) == 0)
            {
                return SPI_Flash_Write(CERT_FLASH_START, (uint8_t *)certificateValue, len);
            }
            else
            {
                LOG_Error("Failed to erase sector. Check that the device is not write protected");
            }
        }
        else
        {
            LOG_Error("Provided certificate value length (%d) exceeds the size of allocated storage (%d)",len,CERT_FLASH_SIZE);
        }
    }
    else
    {
        LOG_Error("Invalid null param");
    }
    return -1;
}


//////// [RE:added]
/**
 * \name   HAL_SetKey
 *
 * \brief  Set the key value in flash
 *
 * \param  keyValue - The key value represented as a null-terminated ASCII string to be programmed into flash
 *
 * \return 0  - success
 *         -1 - failure
 */
int32_t HAL_SetKey(const uint8_t * keyValue)
{
    if (keyValue)
    {
        int len = strlen(keyValue) + 1;

        if (len <= KEY_FLASH_SIZE)
        {
            if (SPI_Flash_Erase(KEY_FLASH_START) == 0)
            {

                return SPI_Flash_Write(KEY_FLASH_START, (uint8_t *)keyValue, len);
            }
            else
            {
                LOG_Error("Failed to erase sector. Check that the device is not write protected");
            }
        }
        else
        {
            LOG_Error("Provided key value length (%d) exceeds the size of allocated storage (%d)",len,KEY_FLASH_SIZE);
        }
    }
    else
    {
        LOG_Error("Invalid null param");
    }
    return -1;
}
#endif
