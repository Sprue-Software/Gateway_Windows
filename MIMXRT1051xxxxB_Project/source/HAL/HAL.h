/*!****************************************************************************
*
* \file HAL.h
*
* \author Evelyne Donnaes
*
* \brief Hardware Abstraction Layer API
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _HAL_H_
#define _HAL_H_

#include <stdint.h>
#include <stdbool.h>

/*!****************************************************************************
 * Constants
 *****************************************************************************/
#define HAL_LEDSTATE_OFF    0
#define HAL_LEDSTATE_ON     1


/* The LEDs on the board
*/

#define HAL_POWER_LED_ID          0x00  // Power is the bottom green LED
#define HAL_ETHERNET_LED_ID       0x01  // Ethernet is the top green LED
#define HAL_CONNECTION_LED_ID     0x02  // Connection is the middle red LED

/* Number of LEDs */
#define HAL_NO_LEDS 3

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/
void HAL_Initialise(void);
void HAL_Reboot();

int16_t HAL_SetLed(uint8_t led, uint8_t value);
int16_t HAL_GetLed(uint8_t led, uint8_t *value);

const char *    HAL_GetMACAddressString(void);
const uint64_t  HAL_GetMACAddress(void);
void HAL_GetMACAddressBytes(const uint8_t* mac);	//////// [RE:added]
uint8_t * HAL_GetRootCA(void);				//////// [RE:added]
uint8_t * HAL_GetCertificate(void);			//////// [RE:added]
uint8_t * HAL_GetKey(void);				//////// [RE:added]
uint8_t * HAL_GetSerialNumber(void);			//////// [RE:added]
uint8_t * HAL_GetWisafeCalibration(void);		//////// [RE:added]
uint8_t HAL_GetActiveImageFlag(void);			//////// [RE:added]
const char * HAL_GetSerialNumberString(void);
void HAL_serialBytes(const uint8_t* mac);
int32_t HAL_SetMacAddress(uint8_t * macAddrValue);	//////// [RE:added]
int32_t HAL_SetRootCA(const uint8_t * rootCaValue);		//////// [RE:added]
int32_t HAL_SetCertificate(const uint8_t * certificateValue);	//////// [RE:added]
int32_t HAL_SetKey(const uint8_t * keyValue);			//////// [RE:added]
int32_t HAL_SetSerialNumber(const uint8_t * serialNum);	//////// [RE:added]
int32_t HAL_SetWisafeCalibration(const uint8_t * wisafeCalValue);	//////// [RE:added]
int32_t HAL_SetActiveImageFlag(uint8_t activeImage);	//////// [RE:added]

int HAL_GetRandom(void * buf, const int size);
int HAL_GetHubID(uint8_t * hub_id, const int size);
/*****************************************************************************/
#endif /* _HAL_H_ */

