################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/DeviceHandlers/TestHandler/THA_Api.c 

OBJS += \
./source/DeviceHandlers/TestHandler/THA_Api.o 

C_DEPS += \
./source/DeviceHandlers/TestHandler/THA_Api.d 


# Each subdirectory must supply rules for building sources it contributes
source/DeviceHandlers/TestHandler/%.o: ../source/DeviceHandlers/TestHandler/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MIMXRT1051CVJ5B -DCPU_MIMXRT1051CVJ5B_cm7 -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DFSL_RTOS_FREE_RTOS -DSDK_OS_FREE_RTOS -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -DMBEDTLS_CONFIG_FILE='"aws_mbedtls_config.h"' -DSERIAL_PORT_TYPE_UART=1 -DUSE_RTOS=1 -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\drivers" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\component\uart" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\drivers\freertos" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\standard\tls\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\pkcs11\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\pkcs11\mbedtls" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\port\arch" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\posix\arpa" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\posix\net" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\posix" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\posix\sys" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\stdc" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\lwip" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\lwip\priv" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\lwip\prot" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\netif" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\netif\ppp" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\netif\ppp\polarssl" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\3rdparty\pkcs11" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\DeviceHandlers\WiSafeHandler" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\freertos_kernel\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\mbedtls\port\ksdk" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\CMSIS" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\secure_sockets\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\template\aws" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\common\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\common\include\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\common\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\mqtt\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\mqtt\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\mqtt\src\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\board" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\standard\pkcs11\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\3rdparty\jsmn" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\utilities" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\standard\utils\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\component\serial_manager" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\freertos\include\platform" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\include\platform" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\standard\crypto\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\device" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\aws\shadow\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\aws\shadow\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\aws\shadow\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\aws\shadow\src\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\serializer\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\component\lists" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\aws\greengrass\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\aws\greengrass\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\mqtt\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\mbedtls\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\freertos\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\port" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\drivers" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\component\uart" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\drivers\freertos" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\standard\tls\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\pkcs11\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\pkcs11\mbedtls" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\port\arch" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\posix\arpa" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\posix\net" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\posix" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\posix\sys" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\compat\stdc" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\lwip" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\lwip\priv" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\lwip\prot" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\netif" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\netif\ppp" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include\netif\ppp\polarssl" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\3rdparty\pkcs11" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\freertos_kernel\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\mbedtls\port\ksdk" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\CMSIS" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\secure_sockets\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\template\aws" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\common\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\common\include\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\common\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\mqtt\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\mqtt\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\mqtt\src\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\board" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\standard\pkcs11\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\3rdparty\jsmn" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\utilities" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\standard\utils\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\component\serial_manager" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\freertos\include\platform" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\include\platform" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\standard\crypto\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\device" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\aws\shadow\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\aws\shadow\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\aws\shadow\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\aws\shadow\src\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\serializer\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\component\lists" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\aws\greengrass\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\freertos_plus\aws\greengrass\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\c_sdk\standard\mqtt\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\mbedtls\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\freertos\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\amazon-freertos\libraries\abstractions\platform\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\port" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\lwip\src\apps\httpsrv" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\Logger" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\wisafe_drv" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\HAL" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\efs" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\spiflash" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\power" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\sntp" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\KeyStore" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\led" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\sto" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\EnsoComms" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\Configuration\RT1050" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\CloudComms\AWS" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\LocalShadow\Api" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\LocalShadow\ObjectStore" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\Applications\Common" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\CloudComms" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\DeviceHandlers\Api" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\DeviceHandlers\Common" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\DeviceHandlers\LEDHandler" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\DeviceHandlers\UpgradeHandler" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\Storage" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\EnsoAgent_Windows\source\OSAL\RT1050\tinyprintf" -O3 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


