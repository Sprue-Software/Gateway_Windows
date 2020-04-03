################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_cache.c \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_dcp.c \
../drivers/fsl_enet.c \
../drivers/fsl_gpio.c \
../drivers/fsl_lpspi.c \
../drivers/fsl_lpuart.c \
../drivers/fsl_trng.c 

OBJS += \
./drivers/fsl_cache.o \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_dcp.o \
./drivers/fsl_enet.o \
./drivers/fsl_gpio.o \
./drivers/fsl_lpspi.o \
./drivers/fsl_lpuart.o \
./drivers/fsl_trng.o 

C_DEPS += \
./drivers/fsl_cache.d \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_dcp.d \
./drivers/fsl_enet.d \
./drivers/fsl_gpio.d \
./drivers/fsl_lpspi.d \
./drivers/fsl_lpuart.d \
./drivers/fsl_trng.d 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT1052DVL6B -DCPU_MIMXRT1052DVL6B_cm7 -D_POSIX_SOURCE -DXIP_BOOT_HEADER_DCD_ENABLE=1 -DSKIP_SYSCLK_INIT -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1 -DPRINTF_ADVANCED_ENABLE=1 -DUSE_RTOS=1 -DLWIP_DNS=1 -DLWIP_DHCP=1 -DMBEDTLS_CONFIG_FILE='"ksdk_mbedtls_config.h"' -DFSL_RTOS_FREE_RTOS -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I../board -I../source -I../ -I../device -I../CMSIS -I../drivers -I../lwip/port/arch -I../lwip/src/include/compat/posix/arpa -I../lwip/src/include/compat/posix/net -I../lwip/src/include/compat/posix -I../lwip/src/include/compat/posix/sys -I../lwip/src/include/compat/stdc -I../lwip/src/include/lwip -I../lwip/src/include/lwip/priv -I../lwip/src/include/lwip/prot -I../lwip/src/include/netif -I../lwip/src/include/netif/ppp -I../lwip/src/include/netif/ppp/polarssl -I../lwip/port -I../mbedtls/port/ksdk -I../amazon-freertos/freertos/portable -I../amazon-freertos/include -I../utilities -I../component/lists -I../component/serial_manager -I../component/uart -I../xip -I../lwip_httpscli_mbedTLS -I../lwip/src -I../lwip/src/include -I../mbedtls/include -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\app\pds" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\wfx_fmac_driver" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\wfx_fmac_driver\bus" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\wfx_fmac_driver\firmware" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\wfx_fmac_driver\firmware\3.3.1" -I"C:\SM_Work\MCUXpresso_IDE_Workspaces\WF200_RefProject\WF200_RefProjectToPort\WF200\app\tcpecho_raw" -O0 -fno-common -g1 -Wall -fomit-frame-pointer  -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


