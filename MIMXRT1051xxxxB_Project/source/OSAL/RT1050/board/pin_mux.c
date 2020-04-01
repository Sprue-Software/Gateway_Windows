/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2016, NXP Semiconductors, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "fsl_gpio.h"


static gpio_pin_config_t gpio_config_output = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
static gpio_pin_config_t gpio_config_input = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};


/*******************************************************************************
 * Code
 ******************************************************************************/

/* Function Name : BOARD_InitPins */
void BOARD_InitPins(void)
{
  bool platform2C = true;
  CLOCK_EnableClock(kCLOCK_Iomuxc);          /* iomuxc clock (iomuxc_clk_enable): 0x03u */



  /* Initialize the LED pins below. */
  IOMUXC_SetPinMux(
      IOMUXC_GPIO_AD_B0_09_GPIO1_IO09,        /* GPIO_AD_B0_09 is configured as GPIO1_IO09 */
      0U);                                    /* Software Input On Field: Input Path is determined by functionality */
  IOMUXC_SetPinMux(
      IOMUXC_GPIO_B1_00_GPIO2_IO16,           /* GPIO_B1_00 is configured as GPIO2_IO16 */
      0U);                                    /* Software Input On Field: Input Path is determined by functionality */
  IOMUXC_SetPinMux(
      IOMUXC_GPIO_B1_01_GPIO2_IO17,           /* GPIO_B1_01 is configured as GPIO2_IO17 */
      0U);                                    /* Software Input On Field: Input Path is determined by functionality */
  IOMUXC_SetPinMux(
      IOMUXC_GPIO_B1_02_GPIO2_IO18,           /* GPIO_B1_02 is configured as GPIO2_IO18 */
      0U);                                    /* Software Input On Field: Input Path is determined by functionality */
  IOMUXC_SetPinConfig(
      IOMUXC_GPIO_AD_B0_09_GPIO1_IO09,        /* GPIO_AD_B0_09 PAD functional properties : */
      0x10B0u);                               /* Slew Rate Field: Slow Slew Rate
                                                 Drive Strength Field: R0/6
                                                 Speed Field: medium(100MHz)
                                                 Open Drain Enable Field: Open Drain Disabled
                                                 Pull / Keep Enable Field: Pull/Keeper Enabled
                                                 Pull / Keep Select Field: Keeper
                                                 Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                 Hyst. Enable Field: Hysteresis Disabled */
  IOMUXC_SetPinConfig(
      IOMUXC_GPIO_B1_00_GPIO2_IO16,           /* GPIO_B1_00 PAD functional properties : */
      0x10B0u);                               /* Slew Rate Field: Slow Slew Rate
                                                 Drive Strength Field: R0/6
                                                 Speed Field: medium(100MHz)
                                                 Open Drain Enable Field: Open Drain Disabled
                                                 Pull / Keep Enable Field: Pull/Keeper Enabled
                                                 Pull / Keep Select Field: Keeper
                                                 Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                 Hyst. Enable Field: Hysteresis Disabled */
  IOMUXC_SetPinConfig(
      IOMUXC_GPIO_B1_01_GPIO2_IO17,           /* GPIO_B1_01 PAD functional properties : */
      0x10B0u);                               /* Slew Rate Field: Slow Slew Rate
                                                 Drive Strength Field: R0/6
                                                 Speed Field: medium(100MHz)
                                                 Open Drain Enable Field: Open Drain Disabled
                                                 Pull / Keep Enable Field: Pull/Keeper Enabled
                                                 Pull / Keep Select Field: Keeper
                                                 Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                 Hyst. Enable Field: Hysteresis Disabled */
  IOMUXC_SetPinConfig(
      IOMUXC_GPIO_B1_02_GPIO2_IO18,           /* GPIO_B1_02 PAD functional properties : */
      0x10B0u);                               /* Slew Rate Field: Slow Slew Rate
                                                 Drive Strength Field: R0/6
                                                 Speed Field: medium(100MHz)
                                                 Open Drain Enable Field: Open Drain Disabled
                                                 Pull / Keep Enable Field: Pull/Keeper Enabled
                                                 Pull / Keep Select Field: Keeper
                                                 Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                 Hyst. Enable Field: Hysteresis Disabled */



  /* Initialize the ENET_ENABLE pin below. */
  IOMUXC_SetPinMux(
      IOMUXC_GPIO_AD_B0_14_GPIO1_IO14,        /* GPIO_AD_B0_14 is configured as GPIO1_IO14 */
      0U);                                    /* Software Input On Field: Input Path is determined by functionality */



  /* Initialize the UART pins below. */
  IOMUXC_SetPinMux(
      IOMUXC_GPIO_AD_B0_12_LPUART1_TX,        /* GPIO_AD_B0_12 is configured as LPUART1_TX */
      0U);                                    /* Software Input On Field: Input Path is determined by functionality */
  IOMUXC_SetPinMux(
      IOMUXC_GPIO_AD_B0_13_LPUART1_RX,        /* GPIO_AD_B0_13 is configured as LPUART1_RX */
      0U);                                    /* Software Input On Field: Input Path is determined by functionality */
  IOMUXC_SetPinConfig(
      IOMUXC_GPIO_AD_B0_12_LPUART1_TX,        /* GPIO_AD_B0_12 PAD functional properties : */
      0x10B0u);                               /* Slew Rate Field: Slow Slew Rate
                                                 Drive Strength Field: R0/6
                                                 Speed Field: medium(100MHz)
                                                 Open Drain Enable Field: Open Drain Disabled
                                                 Pull / Keep Enable Field: Pull/Keeper Enabled
                                                 Pull / Keep Select Field: Keeper
                                                 Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                 Hyst. Enable Field: Hysteresis Disabled */
  IOMUXC_SetPinConfig(
      IOMUXC_GPIO_AD_B0_13_LPUART1_RX,        /* GPIO_AD_B0_13 PAD functional properties : */
      0x10B0u);                               /* Slew Rate Field: Slow Slew Rate
                                                 Drive Strength Field: R0/6
                                                 Speed Field: medium(100MHz)
                                                 Open Drain Enable Field: Open Drain Disabled
                                                 Pull / Keep Enable Field: Pull/Keeper Enabled
                                                 Pull / Keep Select Field: Keeper
                                                 Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                 Hyst. Enable Field: Hysteresis Disabled */



  ////////////////GPIO Configuration/////////////////////

  //SDN Radio Chip Shut Down
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_AD_B1_10_GPIO1_IO26,        /* GPIO_AD_B1_10 is configured as GPIO1_IO26 */
        0U);

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_AD_B1_10_GPIO1_IO26,
        0x10B0U);


  //NIRQ Radio Chip Interrupt Request
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_AD_B1_11_GPIO1_IO27,        /* GPIO_AD_B1_11 is configured as GPIO1_IO27 */
        0U);

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_AD_B1_11_GPIO1_IO27,
        0x10B0U);


  //SPI3 NSEL
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_AD_B1_12_GPIO1_IO28,        /* GPIO_AD_B1_12 is configured as GPIO1_IO28 */
        0U);

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_AD_B1_12_GPIO1_IO28,
        0x10B0U);


  //CTS Radio Chip Clear to Send Signal
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_SD_B0_05_GPIO3_IO17,        /* GPIO_SD_B0_05 is configured as GPIO3_IO17 */
        0U);                                    /* Software Input On Field: Input Path is determined by functionality */

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_SD_B0_05_GPIO3_IO17,
        0x3000U);                             /* Slew Rate Field: Slow Slew Rate
                                                   Drive Strength Field: Output Driver Disabled
                                                   Speed Field: low(50MHz)
                                                   Open Drain Enable Field: Open Drain Disabled
                                                   Pull / Keep Enable Field: Pull/Keeper Enabled
                                                   Pull / Keep Select Field: Pull
                                                   Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                   Hyst. Enable Field: Hysteresis Disabled */


  //SYNC Radio Chip Sync Detect
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_SD_B0_04_GPIO3_IO16,        /* GPIO_SD_B0_04 is configured as GPIO3_IO16 */
        0U);                                    /* Software Input On Field: Input Path is determined by functionality */

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_SD_B0_04_GPIO3_IO16,
        0x3000U);                             /* Slew Rate Field: Slow Slew Rate
                                                   Drive Strength Field: Output Driver Disabled
                                                   Speed Field: low(50MHz)
                                                   Open Drain Enable Field: Open Drain Disabled
                                                   Pull / Keep Enable Field: Pull/Keeper Enabled
                                                   Pull / Keep Select Field: Pull
                                                   Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                   Hyst. Enable Field: Hysteresis Disabled */
#if LEARN_IN_BUTTON
  //User Button Interrupt Request
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_B1_03_GPIO2_IO19,         /* GPIO_B1_03 is configured as GPIO2_IO19 */
        0U);

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_B1_03_GPIO2_IO19,         /* GPIO_B1_03 is configured as GPIO2_IO19 */
        0x10000U);                            /* Slew Rate Field: Slow Slew Rate
                                                   Drive Strength Field: Output Driver Disabled
                                                   Speed Field: low(50MHz)
                                                   Open Drain Enable Field: Open Drain Disabled
                                                   Pull / Keep Enable Field: Pull/Keeper Disabled
                                                   Pull / Keep Select Field: Keeper
                                                   Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                   Hyst. Enable Field: Hysteresis Enabled */
#endif

  //////////////SPI3 Configuration///////////////////////

  //SPI3 SDI
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_AD_B1_13_LPSPI3_SDI,        /* GPIO_AD_B1_13 is configured as LPSPI3_SDI */
        0U);                                    /* Software Input On Field: Input Path is determined by functionality */

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_AD_B1_13_LPSPI3_SDI,        /* GPIO_AD_B1_13 is configured as LPSPI3_SDI */
        0x10B0U);               /* Slew Rate Field: Slow Slew Rate
                                                   Drive Strength Field: R0/6
                                                   Speed Field: medium(100MHz)
                                                   Open Drain Enable Field: Open Drain Disabled
                                                   Pull / Keep Enable Field: Pull/Keeper Enabled
                                                   Pull / Keep Select Field: Keeper
                                                   Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                   Hyst. Enable Field: Hysteresis Disabled */


  //SPI3 SDO
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_AD_B1_14_LPSPI3_SDO,        /* GPIO_AD_B1_14 is configured as LPSPI3_SDO */
        0U);                                    /* Software Input On Field: Input Path is determined by functionality */

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_AD_B1_14_LPSPI3_SDO,        /* GPIO_AD_B1_14 is configured as LPSPI3_SDO */
        0x10B0U);                               /* Software Input On Field: Input Path is determined by functionality */


  //SPI3 Clock
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_AD_B1_15_LPSPI3_SCK,        /* GPIO_AD_B1_15 is configured as LPSPI3_SCK */
        0U);                                    /* Software Input On Field: Input Path is determined by functionality */

  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_AD_B1_15_LPSPI3_SCK,        /* GPIO_AD_B1_15 is configured as LPSPI3_SCK */
        0x10B0U);                               /* Software Input On Field: Input Path is determined by functionality */

  /////////////////////////////////////////////////////////

                                               

  /* Initialize the FlexSPI pins below. */
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_06_FLEXSPIA_SS0_B, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_07_FLEXSPIA_SCLK, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_04_FLEXSPIB_SCLK, 1U);       
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_05_FLEXSPIA_DQS, 1U);             
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_08_FLEXSPIA_DATA00, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_09_FLEXSPIA_DATA01, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_10_FLEXSPIA_DATA02, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_11_FLEXSPIA_DATA03, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_03_FLEXSPIB_DATA00, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_02_FLEXSPIB_DATA01, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_01_FLEXSPIB_DATA02, 1U); 
  IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_00_FLEXSPIB_DATA03, 1U);       
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_06_FLEXSPIA_SS0_B, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_07_FLEXSPIA_SCLK, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_04_FLEXSPIB_SCLK, 0x10F1u);       
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_05_FLEXSPIA_DQS, 0x0130F1u);             
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_08_FLEXSPIA_DATA00, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_09_FLEXSPIA_DATA01, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_10_FLEXSPIA_DATA02, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_11_FLEXSPIA_DATA03, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_03_FLEXSPIB_DATA00, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_02_FLEXSPIB_DATA01, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_01_FLEXSPIB_DATA02, 0x10F1u); 
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_00_FLEXSPIB_DATA03, 0x10F1u);                                           

  // Ethernet PHY RESET pin
  IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09, 0);
  IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) | 
        IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
        IOMUXC_SW_PAD_CTL_PAD_PKE_MASK); 

  // Ethernet PHY INTERRUPT pin pull up
  IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_10_GPIO1_IO10, 0);
  IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_10_GPIO1_IO10, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) | 
        IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
        IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

  //Initialize the PLATFORM DIFFERENTIATOR pin to determine whether we are a WG-2R or WG-2C
  IOMUXC_SetPinMux(
        IOMUXC_GPIO_SD_B0_02_GPIO3_IO14,      /* GPIO_SD_B0_02 is configured as GPIO3_IO14 */
        0U);                                  /* Software Input On Field: Input Path is determined by functionality */
  IOMUXC_SetPinConfig(
        IOMUXC_GPIO_SD_B0_02_GPIO3_IO14,      /* GPIO_SD_B0_02 PAD functional properties : */
        0x40u);                               /* Slew Rate Field: Slow Slew Rate
                                                 Drive Strength Field: Output driver disabled
                                                 Speed Field: medium(100MHz)
                                                 Open Drain Enable Field: Open Drain Disabled
                                                 Pull / Keep Enable Field: Pull/Keeper Disabled
                                                 Pull / Keep Select Field: Keeper
                                                 Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                 Hyst. Enable Field: Hysteresis Disabled */

    GPIO_PinInit(GPIO3, 14, &gpio_config_input);

    // check pin status, 0 = WG-2C, 1 = WG-2R
    if (GPIO_PinRead(GPIO3, 14))
    {
        platform2C = false;
    }

    if (platform2C)
    {
        //Initialize the ENET enable pin
        IOMUXC_SetPinMux(
          IOMUXC_GPIO_AD_B0_14_GPIO1_IO14,        /* GPIO_AD_B0_14 is configured as GPIO1_IO14 */
          0U);                                    /* Software Input On Field: Input Path is determined by functionality */
        IOMUXC_SetPinConfig(
          IOMUXC_GPIO_AD_B0_14_GPIO1_IO14,        /* GPIO_AD_B0_14 PAD functional properties : */
          0x00B0u);                               /* Slew Rate Field: Slow Slew Rate
                                                     Drive Strength Field: R0/6
                                                     Speed Field: medium(100MHz)
                                                     Open Drain Enable Field: Open Drain Disabled
                                                     Pull / Keep Enable Field: Pull/Keeper Disabled
                                                     Pull / Keep Select Field: Keeper
                                                     Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                     Hyst. Enable Field: Hysteresis Disabled */

        // Initialize the USB OTG1 ID pin
        IOMUXC_SetPinMux(
          IOMUXC_GPIO_AD_B0_01_GPIO1_IO01,       /* GPIO_SD_B0_01 is configured as GPIO1_IO01 */
          0U);                                    /* Software Input On Field: Input Path is determined by functionality */
        IOMUXC_SetPinConfig(
          IOMUXC_GPIO_AD_B0_01_GPIO1_IO01,        /* GPIO_SD_B0_01 PAD functional properties : */
          0x1030u);                               /* Slew Rate Field: Slow Slew Rate
                                                     Drive Strength Field: R0/6
                                                     Speed Field: slow(50MHz)
                                                     Open Drain Enable Field: Open Drain Disabled
                                                     Pull / Keep Enable Field: Pull/Keeper Enabled
                                                     Pull / Keep Select Field: Keeper
                                                     Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                     Hyst. Enable Field: Hysteresis Disabled */

        //Initialize the DC 5V IN SENSE pin
        IOMUXC_SetPinMux(
          IOMUXC_GPIO_SD_B0_00_GPIO3_IO12,        /* GPIO_SD_B0_00 is configured as GPIO3_IO12 */
          0U);                                    /* Software Input On Field: Input Path is determined by functionality */
        IOMUXC_SetPinConfig(
          IOMUXC_GPIO_SD_B0_00_GPIO3_IO12,        /* GPIO_SD_B0_00 PAD functional properties : */
          0x0u);                                  /* Slew Rate Field: Slow Slew Rate
                                                     Drive Strength Field: Output driver disabled
                                                     Speed Field: low(50MHz)
                                                     Open Drain Enable Field: Open Drain Disabled
                                                     Pull / Keep Enable Field: Pull/Keeper Disabled
                                                     Pull / Keep Select Field: Keeper
                                                     Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                     Hyst. Enable Field: Hysteresis Disabled */

#if BATTERY_FITTED
        //Initialize the BATTERY ADC pin
        IOMUXC_SetPinMux(
          IOMUXC_GPIO_AD_B1_08_GPIO1_IO24,        /* GPIO_AD_B0_24 is configured as GPIO1_IO24 */
          0U);                                    /* Software Input On Field: Input Path is determined by functionality */
        IOMUXC_SetPinConfig(
          IOMUXC_GPIO_AD_B1_08_GPIO1_IO24,        /* GPIO_AD_B0_14 PAD functional properties : */
          0x00B0u);                               /* Slew Rate Field: Slow Slew Rate
                                                     Drive Strength Field: Output DriverDisabled
                                                     Speed Field: medium(100MHz)
                                                     Open Drain Enable Field: Open Drain Disabled
                                                     Pull / Keep Enable Field: Pull/Keeper Disabled
                                                     Pull / Keep Select Field: Keeper
                                                     Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                     Hyst. Enable Field: Hysteresis Disabled */

        // Initialize the BATTERY CHARGE enable pin
        IOMUXC_SetPinMux(
          IOMUXC_GPIO_B1_14_GPIO2_IO30,             /* GPIO_B1_14 is configured as GPIO2_IO30 */
          0U);                                      /* Software Input On Field: Input Path is determined by functionality */
        IOMUXC_SetPinConfig(
          IOMUXC_GPIO_B1_14_GPIO2_IO30,             /* GPIO_B1_14 PAD functional properties : */
          0x1030u);                                 /* Slew Rate Field: Slow Slew Rate
                                                       Drive Strength Field: R0/6
                                                       Speed Field: slow(50MHz)
                                                       Open Drain Enable Field: Open Drain Disabled
                                                       Pull / Keep Enable Field: Pull/Keeper Enabled
                                                       Pull / Keep Select Field: Keeper
                                                       Pull Up / Down Config. Field: 100K Ohm Pull Down
                                                       Hyst. Enable Field: Hysteresis Disabled */

#endif
    }
}


static void delay(int n)
{
    volatile uint32_t i = 0;
    for (i = 0; i < n; ++i)
    {
        __asm("NOP"); /* delay */
    }
}


void enable_wired_ethernet(bool enable)
{
    if (enable)
    {
        IOMUXC_EnableMode(IOMUXC_GPR, kIOMUXC_GPR_ENET1TxClkOutputDir, true);

        // Enable power to Ethernet PHY
        GPIO_WritePinOutput(GPIO1, 14, 0);

        // configure Ethernet PHY interface signals
        IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_40_ENET_MDC, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_40_ENET_MDC, IOMUXC_SW_PAD_CTL_PAD_SPEED(3) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
          IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
          IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_41_ENET_MDIO, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_41_ENET_MDIO, IOMUXC_SW_PAD_CTL_PAD_SPEED(0) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
          IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
          IOMUXC_SW_PAD_CTL_PAD_ODE_MASK | IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_10_ENET_REF_CLK, 1);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_10_ENET_REF_CLK, IOMUXC_SW_PAD_CTL_PAD_DSE(6) |IOMUXC_SW_PAD_CTL_PAD_SRE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_04_ENET_RX_DATA00, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_04_ENET_RX_DATA00, IOMUXC_SW_PAD_CTL_PAD_SPEED(3) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
          IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
          IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_05_ENET_RX_DATA01, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_05_ENET_RX_DATA01, IOMUXC_SW_PAD_CTL_PAD_SPEED(3) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
          IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
          IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_06_ENET_RX_EN, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_06_ENET_RX_EN, IOMUXC_SW_PAD_CTL_PAD_SPEED(3) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
          IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
          IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_11_ENET_RX_ER, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_11_ENET_RX_ER, IOMUXC_SW_PAD_CTL_PAD_SPEED(3) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
          IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
          IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_07_ENET_TX_DATA00, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_07_ENET_TX_DATA00, IOMUXC_SW_PAD_CTL_PAD_SPEED(0) | IOMUXC_SW_PAD_CTL_PAD_DSE(3) |
          IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_08_ENET_TX_DATA01, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_08_ENET_TX_DATA01, IOMUXC_SW_PAD_CTL_PAD_SPEED(0) | IOMUXC_SW_PAD_CTL_PAD_DSE(3) |
          IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK | IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_09_ENET_TX_EN, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_09_ENET_TX_EN, IOMUXC_SW_PAD_CTL_PAD_SPEED(3) | IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
          IOMUXC_SW_PAD_CTL_PAD_SRE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(2) | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
          IOMUXC_SW_PAD_CTL_PAD_PKE_MASK);

        // Setup Ethernet PHY control signals
        GPIO_PinInit(GPIO1, 9, &gpio_config_output);
        GPIO_PinInit(GPIO1, 10, &gpio_config_output);

        // pull up the ENET_INT before RESET.
        GPIO_WritePinOutput(GPIO1, 10, 1);
        GPIO_WritePinOutput(GPIO1, 9, 0);

        delay(100000);
        // release Ethernet PHY reset
        GPIO_WritePinOutput(GPIO1, 9, 1);
    }
    else
    {
        // remove power from Ethernet PHY
        GPIO_WritePinOutput(GPIO1, 14, 1);

        IOMUXC_EnableMode(IOMUXC_GPR, kIOMUXC_GPR_ENET1TxClkOutputDir, false);

        // reconfigure Ethernet PHY interface signals as inputs
        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_04_GPIO2_IO20, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_04_GPIO2_IO20, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO2, 20, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_05_GPIO2_IO21, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_05_GPIO2_IO21, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO2, 21, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_06_GPIO2_IO22, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_06_GPIO2_IO22, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO2, 22, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_07_GPIO2_IO23, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_07_GPIO2_IO23, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO2, 23, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_08_GPIO2_IO24, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_08_GPIO2_IO24, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO2, 24, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_40_GPIO3_IO26, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_40_GPIO3_IO26, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO3, 26, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_41_GPIO3_IO27, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_41_GPIO3_IO27, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO3, 27, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_09_GPIO2_IO25, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_09_GPIO2_IO25, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO2, 25, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_10_GPIO2_IO26, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_10_GPIO2_IO26, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO2, 26, &gpio_config_input);

        IOMUXC_SetPinMux(IOMUXC_GPIO_B1_11_GPIO2_IO27, 0);
        IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_11_GPIO2_IO27, IOMUXC_SW_PAD_CTL_PAD_SPEED(1) | IOMUXC_SW_PAD_CTL_PAD_DSE(5));
        GPIO_PinInit(GPIO2, 27, &gpio_config_input);

        GPIO_PinInit(GPIO1, 9, &gpio_config_input);
        GPIO_PinInit(GPIO1, 10, &gpio_config_input);
    }
}
