################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/src/netif/ppp/auth.c \
../lwip/src/netif/ppp/ccp.c \
../lwip/src/netif/ppp/chap-md5.c \
../lwip/src/netif/ppp/chap-new.c \
../lwip/src/netif/ppp/chap_ms.c \
../lwip/src/netif/ppp/demand.c \
../lwip/src/netif/ppp/eap.c \
../lwip/src/netif/ppp/eui64.c \
../lwip/src/netif/ppp/fsm.c \
../lwip/src/netif/ppp/ipcp.c \
../lwip/src/netif/ppp/ipv6cp.c \
../lwip/src/netif/ppp/lcp.c \
../lwip/src/netif/ppp/lwip_ecp.c \
../lwip/src/netif/ppp/magic.c \
../lwip/src/netif/ppp/mppe.c \
../lwip/src/netif/ppp/multilink.c \
../lwip/src/netif/ppp/ppp.c \
../lwip/src/netif/ppp/pppapi.c \
../lwip/src/netif/ppp/pppcrypt.c \
../lwip/src/netif/ppp/pppoe.c \
../lwip/src/netif/ppp/pppol2tp.c \
../lwip/src/netif/ppp/pppos.c \
../lwip/src/netif/ppp/upap.c \
../lwip/src/netif/ppp/utils.c \
../lwip/src/netif/ppp/vj.c 

OBJS += \
./lwip/src/netif/ppp/auth.o \
./lwip/src/netif/ppp/ccp.o \
./lwip/src/netif/ppp/chap-md5.o \
./lwip/src/netif/ppp/chap-new.o \
./lwip/src/netif/ppp/chap_ms.o \
./lwip/src/netif/ppp/demand.o \
./lwip/src/netif/ppp/eap.o \
./lwip/src/netif/ppp/eui64.o \
./lwip/src/netif/ppp/fsm.o \
./lwip/src/netif/ppp/ipcp.o \
./lwip/src/netif/ppp/ipv6cp.o \
./lwip/src/netif/ppp/lcp.o \
./lwip/src/netif/ppp/lwip_ecp.o \
./lwip/src/netif/ppp/magic.o \
./lwip/src/netif/ppp/mppe.o \
./lwip/src/netif/ppp/multilink.o \
./lwip/src/netif/ppp/ppp.o \
./lwip/src/netif/ppp/pppapi.o \
./lwip/src/netif/ppp/pppcrypt.o \
./lwip/src/netif/ppp/pppoe.o \
./lwip/src/netif/ppp/pppol2tp.o \
./lwip/src/netif/ppp/pppos.o \
./lwip/src/netif/ppp/upap.o \
./lwip/src/netif/ppp/utils.o \
./lwip/src/netif/ppp/vj.o 

C_DEPS += \
./lwip/src/netif/ppp/auth.d \
./lwip/src/netif/ppp/ccp.d \
./lwip/src/netif/ppp/chap-md5.d \
./lwip/src/netif/ppp/chap-new.d \
./lwip/src/netif/ppp/chap_ms.d \
./lwip/src/netif/ppp/demand.d \
./lwip/src/netif/ppp/eap.d \
./lwip/src/netif/ppp/eui64.d \
./lwip/src/netif/ppp/fsm.d \
./lwip/src/netif/ppp/ipcp.d \
./lwip/src/netif/ppp/ipv6cp.d \
./lwip/src/netif/ppp/lcp.d \
./lwip/src/netif/ppp/lwip_ecp.d \
./lwip/src/netif/ppp/magic.d \
./lwip/src/netif/ppp/mppe.d \
./lwip/src/netif/ppp/multilink.d \
./lwip/src/netif/ppp/ppp.d \
./lwip/src/netif/ppp/pppapi.d \
./lwip/src/netif/ppp/pppcrypt.d \
./lwip/src/netif/ppp/pppoe.d \
./lwip/src/netif/ppp/pppol2tp.d \
./lwip/src/netif/ppp/pppos.d \
./lwip/src/netif/ppp/upap.d \
./lwip/src/netif/ppp/utils.d \
./lwip/src/netif/ppp/vj.d 


# Each subdirectory must supply rules for building sources it contributes
lwip/src/netif/ppp/%.o: ../lwip/src/netif/ppp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MIMXRT1051CVJ5B -DCPU_MIMXRT1051CVJ5B_cm7 -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DFSL_RTOS_FREE_RTOS -DSDK_OS_FREE_RTOS -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -DMBEDTLS_CONFIG_FILE='"aws_mbedtls_config.h"' -DSERIAL_PORT_TYPE_UART=1 -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\drivers" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\component\uart" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\drivers\freertos" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\freertos_plus\standard\tls\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\template\aws" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\CMSIS" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\common\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\common\include\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\common\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\mqtt\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\mqtt\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\mqtt\src\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\board" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\source" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\pkcs11\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\pkcs11\mbedtls" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\freertos_plus\standard\pkcs11\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\port\arch" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\posix\arpa" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\posix\net" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\posix" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\posix\sys" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\stdc" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\lwip" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\lwip\priv" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\lwip\prot" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\netif" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\netif\ppp" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\netif\ppp\polarssl" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\3rdparty\jsmn" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\utilities" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\freertos_plus\standard\utils\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\component\serial_manager" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\3rdparty\pkcs11" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\freertos\include\platform" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\include\platform" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\freertos_plus\standard\crypto\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\freertos_kernel\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\mbedtls\port\ksdk" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\device" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\secure_sockets\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\aws\shadow\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\aws\shadow\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\aws\shadow\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\aws\shadow\src\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\serializer\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\component\lists" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\mqtt\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\mbedtls\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\freertos\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\port" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\drivers" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\component\uart" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\drivers\freertos" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\freertos_plus\standard\tls\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\pkcs11\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\pkcs11\mbedtls" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\port\arch" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\posix\arpa" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\posix\net" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\posix" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\posix\sys" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\compat\stdc" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\lwip" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\lwip\priv" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\lwip\prot" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\netif" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\netif\ppp" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include\netif\ppp\polarssl" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\3rdparty\pkcs11" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\freertos_kernel\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\mbedtls\port\ksdk" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\CMSIS" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\secure_sockets\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\template\aws" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\board" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\source" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\freertos_plus\standard\pkcs11\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\3rdparty\jsmn" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\utilities" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\freertos_plus\standard\utils\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\component\serial_manager" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\freertos\include\platform" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\include\platform" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\freertos_plus\standard\crypto\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\device" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\aws\shadow\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\aws\shadow\include\types" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\aws\shadow\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\aws\shadow\src\private" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\c_sdk\standard\serializer\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\component\lists" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\mbedtls\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\freertos\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\amazon-freertos\libraries\abstractions\platform\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\port" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\include" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project\lwip\src\apps\httpsrv" -I"C:\Users\ndiwathe\Documents\MCUXpressoIDE_11.1.1_3241\workspace\MIMXRT1051xxxxB_Project" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


