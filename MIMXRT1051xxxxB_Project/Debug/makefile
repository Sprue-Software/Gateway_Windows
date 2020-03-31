################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include mbedtls/port/ksdk/subdir.mk
-include mbedtls/library/subdir.mk
-include lwip/src/netif/ppp/subdir.mk
-include lwip/src/netif/subdir.mk
-include lwip/src/core/ipv6/subdir.mk
-include lwip/src/core/ipv4/subdir.mk
-include lwip/src/core/subdir.mk
-include lwip/src/api/subdir.mk
-include lwip/port/subdir.mk
-include drivers/freertos/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/lists/subdir.mk
-include board/subdir.mk
-include amazon-freertos/vendors/nxp/secure_sockets/lwip/subdir.mk
-include amazon-freertos/vendors/nxp/pkcs11/common/subdir.mk
-include amazon-freertos/vendors/nxp/mflash/mimxrt1052/subdir.mk
-include amazon-freertos/libraries/freertos_plus/standard/utils/src/subdir.mk
-include amazon-freertos/libraries/freertos_plus/standard/tls/src/subdir.mk
-include amazon-freertos/libraries/freertos_plus/standard/pkcs11/src/subdir.mk
-include amazon-freertos/libraries/freertos_plus/standard/crypto/src/subdir.mk
-include amazon-freertos/libraries/c_sdk/standard/serializer/src/json/subdir.mk
-include amazon-freertos/libraries/c_sdk/standard/serializer/src/subdir.mk
-include amazon-freertos/libraries/c_sdk/standard/mqtt/src/subdir.mk
-include amazon-freertos/libraries/c_sdk/standard/common/taskpool/subdir.mk
-include amazon-freertos/libraries/c_sdk/standard/common/logging/subdir.mk
-include amazon-freertos/libraries/c_sdk/standard/common/subdir.mk
-include amazon-freertos/libraries/c_sdk/aws/shadow/src/subdir.mk
-include amazon-freertos/libraries/abstractions/platform/freertos/subdir.mk
-include amazon-freertos/libraries/abstractions/pkcs11/mbedtls/subdir.mk
-include amazon-freertos/libraries/3rdparty/jsmn/subdir.mk
-include amazon-freertos/freertos_kernel/portable/MemMang/subdir.mk
-include amazon-freertos/freertos_kernel/portable/GCC/ARM_CM4F/subdir.mk
-include amazon-freertos/freertos_kernel/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: MIMXRT1051xxxxB_Project.axf

# Tool invocations
MIMXRT1051xxxxB_Project.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="MIMXRT1051xxxxB_Project.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -T MIMXRT1051xxxxB_Project_Debug.ld -o "MIMXRT1051xxxxB_Project.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) MIMXRT1051xxxxB_Project.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "MIMXRT1051xxxxB_Project.axf"; # arm-none-eabi-objcopy -v -O binary "MIMXRT1051xxxxB_Project.axf" "MIMXRT1051xxxxB_Project.bin" ; # checksum -p MIMXRT1051xxxxB -d "MIMXRT1051xxxxB_Project.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
