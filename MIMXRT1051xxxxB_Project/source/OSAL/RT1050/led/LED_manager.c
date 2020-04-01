/*!****************************************************************************
 *
 * \file LED_manager.c
 *
 * \author Guy Bellini
 *
 * \brief Manage LED functionality
 *
 * \Copyright (C) 2018 Fireangel Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdint.h>
//#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "HAL.h"
#include "OSAL_Api.h"
#include "LOG_Api.h"
#include "LED_manager.h"
#include "power.h"
#include "C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\wisafe_drv\wisafe_main.h"

#include "fsl_gpio.h"
#include "board.h"

#include "sntp.h"


extern bool network_link_status(void);
extern bool gatewayTypeWG2C(void);

/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

/*!****************************************************************************
 * Constants
 *****************************************************************************/


/*!****************************************************************************
 * Variables
 *****************************************************************************/

/* Enso agent requested LED state */
static uint8_t enso_led_state[HAL_NO_LEDS];

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/


/**
 * \name   set_Led
 *
 * \brief  Turn the selected LED on or off
 *
 * \param  led   - The LED number
 *
 * \param  value - 0 = off, 1 = 0n
 *
 * \return none
 */
static void set_Led(uint8_t led, uint8_t value)
{
    // Switch LED on board
    switch(led)
    {
#if 0 //nishi
        case POWER_LED_ID:
            GPIO_PinWrite(BOARD_POWER_LED_GPIO, BOARD_POWER_LED_GPIO_PIN, (value ? 0U : 1U));
            break;
        case ETHERNET_LED_ID:
            GPIO_PinWrite(BOARD_CONN_LED_GPIO, BOARD_CONN_LED_GPIO_PIN, (value ? 0U : 1U));
            break;
        case CONNECTION_LED_ID:
            GPIO_PinWrite(BOARD_FAULT_LED_GPIO, BOARD_FAULT_LED_GPIO_PIN, (value ? 0U : 1U));
            break;
#endif
        default:
            break;
    }
}


/*!
 * @brief Function to manage LEDs
 */
static void led_manager(void *arg)
{
    uint32_t led_state[NUM_LEDS];
    uint32_t tick100ms = 0;
    learn_state_type_t wisafeLearnInStatus = LEARN_INACTIVE;

    set_Led(POWER_LED_ID,LEDSTATE_ON);
    set_Led(ETHERNET_LED_ID,LEDSTATE_OFF);
    set_Led(CONNECTION_LED_ID,LEDSTATE_OFF);

    while (1)
    {
        tick100ms++;
        if (tick100ms >= 10)
        {
            tick100ms = 0;
        }

        led_state[POWER_LED_ID] = LEDSTATE_OFF;
        led_state[ETHERNET_LED_ID] = LEDSTATE_OFF;
        led_state[CONNECTION_LED_ID] = LEDSTATE_OFF;

#if LEARN_IN_BUTTON
     //   wisafeLearnInStatus = getWisafeLearnInStatus();
#else
        wisafeLearnInStatus = LEARN_INACTIVE;
#endif

        if (wisafeLearnInStatus == LEARN_ACTIVE) //nishi
        {
            led_state[POWER_LED_ID] = LEDSTATE_ON;
            led_state[ETHERNET_LED_ID] = LEDSTATE_ON;
            led_state[CONNECTION_LED_ID] = LEDSTATE_ON;
        }
        else
        {
            if (wisafeLearnInStatus != LEARN_INACTIVE)
            {
                switch (wisafeLearnInStatus)
                {
                    case LEARN_ACTIVE:
                        led_state[POWER_LED_ID] = LEDSTATE_ON;
                        led_state[ETHERNET_LED_ID] = LEDSTATE_ON;
                        led_state[CONNECTION_LED_ID] = LEDSTATE_ON;
                        break;

                    case LEARN_JOINED:
                        resetWisafeLearnInStatus();
                        for(uint8_t i=0; i<5; i++)
                        {
                            set_Led(POWER_LED_ID,LEDSTATE_ON);
                            set_Led(ETHERNET_LED_ID,LEDSTATE_ON);
                            set_Led(CONNECTION_LED_ID,LEDSTATE_ON);
                            OSAL_sleep_ms(500);
                            set_Led(POWER_LED_ID,LEDSTATE_OFF);
                            set_Led(ETHERNET_LED_ID,LEDSTATE_OFF);
                            set_Led(CONNECTION_LED_ID,LEDSTATE_OFF);
                            OSAL_sleep_ms(500);
                            tick100ms = 0;
                        }
                        break;

                    case LEARN_UNLEARNT:
                        resetWisafeLearnInStatus();
                        for(uint8_t i=0; i<2; i++)
                        {
                            set_Led(POWER_LED_ID,LEDSTATE_ON);
                            set_Led(ETHERNET_LED_ID,LEDSTATE_ON);
                            set_Led(CONNECTION_LED_ID,LEDSTATE_ON);
                            OSAL_sleep_ms(500);
                            set_Led(POWER_LED_ID,LEDSTATE_OFF);
                            set_Led(ETHERNET_LED_ID,LEDSTATE_OFF);
                            set_Led(CONNECTION_LED_ID,LEDSTATE_OFF);
                            OSAL_sleep_ms(1000);
                        }
                        for(uint8_t i=0; i<3; i++)
                        {
                            set_Led(POWER_LED_ID,LEDSTATE_ON);
                            set_Led(ETHERNET_LED_ID,LEDSTATE_ON);
                            set_Led(CONNECTION_LED_ID,LEDSTATE_ON);
                            OSAL_sleep_ms(120);
                            set_Led(POWER_LED_ID,LEDSTATE_OFF);
                            set_Led(ETHERNET_LED_ID,LEDSTATE_OFF);
                            set_Led(CONNECTION_LED_ID,LEDSTATE_OFF);
                            OSAL_sleep_ms(120);
                        }
                        tick100ms = 0;
                        break;

                    default:
                        LOG_Error("Unrecognised learn-in status");
                        resetWisafeLearnInStatus();
                        break;
                }
            }
            else
            {
                if (gatewayTypeWG2C())
                {
                    if (isMainsConnected())
                    {
                        led_state[POWER_LED_ID] = LEDSTATE_ON;
                    }
                    else
                    {
                        if (tick100ms < 5)
                        {
                            led_state[POWER_LED_ID] = LEDSTATE_ON;
                        }
                    }
                }
                else
                {
                    led_state[POWER_LED_ID] = LEDSTATE_ON;
                }

                led_state[CONNECTION_LED_ID] = enso_led_state[CONNECTION_LED_ID];
//                led_state[ETHERNET_LED_ID] = enso_led_state[ETHERNET_LED_ID];
                //illuminate connection LED if usb or ethernet link up - ignore enso control
         //nishi       if (network_link_status())
                //{
                //    led_state[ETHERNET_LED_ID] = LEDSTATE_ON;
                //}
            }
        }
        set_Led(POWER_LED_ID,led_state[POWER_LED_ID]);
        set_Led(ETHERNET_LED_ID,led_state[ETHERNET_LED_ID]);
        set_Led(CONNECTION_LED_ID,led_state[CONNECTION_LED_ID]);

        OSAL_sleep_ms(100);
    }
}


/**
 * \name LED_Manager_Init
 *
 * \brief Initialise the LED Manager - power LED on, other LEDs off.
 */
int LED_Manager_Init(void)
{
	//nishi
#if 0
    // Init output LED GPIO.
    gpio_pin_config_t led_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    GPIO_PinInit(BOARD_POWER_LED_GPIO, BOARD_POWER_LED_GPIO_PIN, &led_config);
    GPIO_PinWrite(BOARD_POWER_LED_GPIO, BOARD_POWER_LED_GPIO_PIN,1);
    
    GPIO_PinInit(BOARD_CONN_LED_GPIO, BOARD_CONN_LED_GPIO_PIN, &led_config);
    GPIO_PinWrite(BOARD_CONN_LED_GPIO, BOARD_CONN_LED_GPIO_PIN,1);

    GPIO_PinInit(BOARD_FAULT_LED_GPIO, BOARD_FAULT_LED_GPIO_PIN, &led_config);
    GPIO_PinWrite(BOARD_FAULT_LED_GPIO, BOARD_FAULT_LED_GPIO_PIN,1);
#endif
    for (int led = 0; led < NUM_LEDS; led++)
    {
        enso_led_state[led] = LEDSTATE_OFF;
    }
#define PRIORITIE_OFFSET			( 4 ) //nishi
    // create thread to manage LEDs
    if(xTaskCreate(led_manager, ((const char*)"led_manager"), configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL) != pdPASS)
    {
        LOG_Error("LED manager task creation failed!.");
        return -1;
    }
    return 0;
}


/**
 * \name   LED_Manager_EnsoAgent_SetLed
 *
 * \brief  Turn the selected LED on or off
 *
 * \param  led   - The LED number
 *
 * \param  value - 0 = off, 1 = 0n
 *
 * \return 0 = status, -1 = error
 */
int16_t LED_Manager_EnsoAgent_SetLed(uint8_t led, uint8_t value)
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
    enso_led_state[led] = value;

    return 0;
}


/**
 * \name   LED_Manager_EnsoAgent_GetLed
 *
 * \brief  Get the state of a LED
 *
 * \param  led   - The LED number
 *
 * \param  value - 0 = off, 1 = 0n
 *
 * \return 0 = status, -1 = error
 */
int16_t LED_Manager_EnsoAgent_GetLed(uint8_t led, uint8_t *value)
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

    *value = enso_led_state[led];
    LOG_Info("LED %u state is %u", led, *value);

    return ret;
}
