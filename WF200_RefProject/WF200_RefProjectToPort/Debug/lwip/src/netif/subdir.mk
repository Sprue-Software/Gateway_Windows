################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/src/netif/bridgeif.c \
../lwip/src/netif/bridgeif_fdb.c \
../lwip/src/netif/ethernet.c \
../lwip/src/netif/lowpan6.c \
../lwip/src/netif/lowpan6_ble.c \
../lwip/src/netif/lowpan6_common.c \
../lwip/src/netif/slipif.c \
../lwip/src/netif/zepif.c 

OBJS += \
./lwip/src/netif/bridgeif.o \
./lwip/src/netif/bridgeif_fdb.o \
./lwip/src/netif/ethernet.o \
./lwip/src/netif/lowpan6.o \
./lwip/src/netif/lowpan6_ble.o \
./lwip/src/netif/lowpan6_common.o \
./lwip/src/netif/slipif.o \
./lwip/src/netif/zepif.o 

C_DEPS += \
./lwip/src/netif/bridgeif.d \
./lwip/src/netif/bridgeif_fdb.d \
./lwip/src/netif/ethernet.d \
./lwip/src/netif/lowpan6.d \
./lwip/src/netif/lowpan6_ble.d \
./lwip/src/netif/lowpan6_common.d \
./lwip/src/netif/slipif.d \
./lwip/src/netif/zepif.d 


# Each subdirectory must supply rules for building sources it contributes
lwip/src/netif/%.o: ../lwip/src/netif/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT1052DVL6B -DCPU_MIMXRT1052DVL6B_cm7 -D_POSIX_SOURCE -DXIP_BOOT_HEADER_DCD_ENABLE=1 -DSKIP_SYSCLK_INIT -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1 -DPRINTF_ADVANCED_ENABLE=1 -DUSE_RTOS=1 -DLWIP_DNS=1 -DLWIP_DHCP=1 -DMBEDTLS_CONFIG_FILE='"ksdk_mbedtls_config.h"' -DFSL_RTOS_FREE_RTOS -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I../board -I../source -I../ -I../device -I../CMSIS -I../drivers -I../lwip/port/arch -I../lwip/src/include/compat/posix/arpa -I../lwip/src/include/compat/posix/net -I../lwip/src/include/compat/posix -I../lwip/src/include/compat/posix/sys -I../lwip/src/include/compat/stdc -I../lwip/src/include/lwip -I../lwip/src/include/lwip/priv -I../lwip/src/include/lwip/prot -I../lwip/src/include/netif -I../lwip/src/include/netif/ppp -I../lwip/src/include/netif/ppp/polarssl -I../lwip/port -I../mbedtls/port/ksdk -I../amazon-freertos/freertos/portable -I../amazon-freertos/include -I../utilities -I../component/lists -I../component/serial_manager -I../component/uart -I../xip -I../lwip_httpscli_mbedTLS -I../lwip/src -I../lwip/src/include -I../mbedtls/include -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\app\pds" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\wfx_fmac_driver" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\wfx_fmac_driver\bus" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\wfx_fmac_driver\firmware" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\wfx_fmac_driver\firmware\3.3.1" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\app\tcpecho_raw" -O0 -fno-common -g1 -Wall -fomit-frame-pointer  -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

