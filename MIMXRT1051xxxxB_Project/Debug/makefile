################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/Watchdog/subdir.mk
-include source/Storage/subdir.mk
-include source/OSAL/RT1050/wisafe_drv/subdir.mk
-include source/OSAL/RT1050/watchdog/subdir.mk
-include source/OSAL/RT1050/tinyprintf/subdir.mk
-include source/OSAL/RT1050/sto/subdir.mk
-include source/OSAL/RT1050/spiflash/subdir.mk
-include source/OSAL/RT1050/sntp/subdir.mk
-include source/OSAL/RT1050/power/subdir.mk
-include source/OSAL/RT1050/ota/subdir.mk
-include source/OSAL/RT1050/led/subdir.mk
-include source/OSAL/RT1050/efs/subdir.mk
-include source/OSAL/RT1050/subdir.mk
-include source/Logger/subdir.mk
-include source/LocalShadow/ObjectStore/subdir.mk
-include source/LocalShadow/Api/subdir.mk
-include source/KeyStore/FreeRTOS/subdir.mk
-include source/HAL/RT1050/subdir.mk
-include source/EnsoComms/subdir.mk
-include source/DeviceHandlers/WiSafeHandler/subdir.mk
-include source/DeviceHandlers/UpgradeHandler/RT1050/subdir.mk
-include source/DeviceHandlers/UpgradeHandler/subdir.mk
-include source/DeviceHandlers/TestHandler/subdir.mk
-include source/DeviceHandlers/LEDHandler/subdir.mk
-include source/DeviceHandlers/GatewayHandler/subdir.mk
-include source/DeviceHandlers/Common/subdir.mk
-include source/CloudComms/AWS/subdir.mk
-include source/CloudComms/subdir.mk
-include source/Applications/WisafeGateway/AmebaWISAFESimulation/subdir.mk
-include source/Applications/WisafeGateway/subdir.mk
-include source/Applications/Common/subdir.mk
-include source/subdir.mk
-include mbedtls/port/ksdk/subdir.mk
-include mbedtls/library/subdir.mk
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
-include amazon-freertos/libraries/freertos_plus/aws/greengrass/src/subdir.mk
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
all: EnsoAgent_Windows.axf

# Tool invocations
EnsoAgent_Windows.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="EnsoAgent_Windows.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -T "EnsoAgent_Windows_Debug.ld" -o "EnsoAgent_Windows.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) EnsoAgent_Windows.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "EnsoAgent_Windows.axf"; # arm-none-eabi-objcopy -v -O binary "EnsoAgent_Windows.axf" "EnsoAgent_Windows.bin" ; # checksum -p MIMXRT1051xxxxB -d "EnsoAgent_Windows.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
