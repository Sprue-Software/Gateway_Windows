/*!****************************************************************************
 *
 * \file LED_manager.h
 *
 * \author Guy Bellini
 *
 * \brief Manage LED functionality
 *
 * \Copyright (C) 2018 Fireangel Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#ifndef _LED_manager_H_
#define _LED_manager_H_

#include <stdint.h>
#include <stdbool.h>

/*!****************************************************************************
 * Constants
 *****************************************************************************/
#define LEDSTATE_OFF    0
#define LEDSTATE_ON     1


/* The LEDs on the board
*/

#define POWER_LED_ID          0x00  // Power is the bottom green LED
#define ETHERNET_LED_ID       0x01  // Ethernet is the top green LED
#define CONNECTION_LED_ID     0x02  // Connection is the middle red LED

/* Number of LEDs */
#define NUM_LEDS 3

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/
extern int LED_Manager_Init(void);

int16_t LED_Manager_EnsoAgent_SetLed(uint8_t led, uint8_t value);
int16_t LED_Manager_EnsoAgent_GetLed(uint8_t led, uint8_t *value);

/*****************************************************************************/
#endif /* _LED_manager_H_ */

