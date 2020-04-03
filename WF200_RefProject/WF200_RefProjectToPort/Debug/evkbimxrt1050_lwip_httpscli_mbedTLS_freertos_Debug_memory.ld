/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright (c) 2008-2013 Code Red Technologies Ltd,
 * Copyright 2015, 2018-2019 NXP
 * (c) NXP Semiconductors 2013-2020
 * Generated linker script file for MIMXRT1052xxxxB
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.23
 * MCUXpresso IDE v11.0.1 [Build 2563] [2019-09-01] on 03-Apr-2020 15:38:20
 */

MEMORY
{
  /* Define each memory region */
  BOARD_FLASH (rx) : ORIGIN = 0x60000000, LENGTH = 0x4000000 /* 64M bytes (alias Flash) */  
  BOARD_SDRAM (rwx) : ORIGIN = 0x80000000, LENGTH = 0x1e00000 /* 30M bytes (alias RAM) */  
  BOARD_SDRAM_NONCACHEABLE (rwx) : ORIGIN = 0x81e00000, LENGTH = 0x200000 /* 2M bytes (alias RAM2) */  
  SRAM_DTC (rwx) : ORIGIN = 0x20000000, LENGTH = 0x20000 /* 128K bytes (alias RAM3) */  
  SRAM_ITC (rwx) : ORIGIN = 0x0, LENGTH = 0x20000 /* 128K bytes (alias RAM4) */  
  SRAM_OC (rwx) : ORIGIN = 0x20200000, LENGTH = 0xc0000 /* 768K bytes (alias RAM5) */  
}

  /* Define a symbol for the top of each memory region */
  __base_BOARD_FLASH = 0x60000000  ; /* BOARD_FLASH */  
  __base_Flash = 0x60000000 ; /* Flash */  
  __top_BOARD_FLASH = 0x60000000 + 0x4000000 ; /* 64M bytes */  
  __top_Flash = 0x60000000 + 0x4000000 ; /* 64M bytes */  
  __base_BOARD_SDRAM = 0x80000000  ; /* BOARD_SDRAM */  
  __base_RAM = 0x80000000 ; /* RAM */  
  __top_BOARD_SDRAM = 0x80000000 + 0x1e00000 ; /* 30M bytes */  
  __top_RAM = 0x80000000 + 0x1e00000 ; /* 30M bytes */  
  __base_BOARD_SDRAM_NONCACHEABLE = 0x81e00000  ; /* BOARD_SDRAM_NONCACHEABLE */  
  __base_RAM2 = 0x81e00000 ; /* RAM2 */  
  __top_BOARD_SDRAM_NONCACHEABLE = 0x81e00000 + 0x200000 ; /* 2M bytes */  
  __top_RAM2 = 0x81e00000 + 0x200000 ; /* 2M bytes */  
  __base_SRAM_DTC = 0x20000000  ; /* SRAM_DTC */  
  __base_RAM3 = 0x20000000 ; /* RAM3 */  
  __top_SRAM_DTC = 0x20000000 + 0x20000 ; /* 128K bytes */  
  __top_RAM3 = 0x20000000 + 0x20000 ; /* 128K bytes */  
  __base_SRAM_ITC = 0x0  ; /* SRAM_ITC */  
  __base_RAM4 = 0x0 ; /* RAM4 */  
  __top_SRAM_ITC = 0x0 + 0x20000 ; /* 128K bytes */  
  __top_RAM4 = 0x0 + 0x20000 ; /* 128K bytes */  
  __base_SRAM_OC = 0x20200000  ; /* SRAM_OC */  
  __base_RAM5 = 0x20200000 ; /* RAM5 */  
  __top_SRAM_OC = 0x20200000 + 0xc0000 ; /* 768K bytes */  
  __top_RAM5 = 0x20200000 + 0xc0000 ; /* 768K bytes */  
