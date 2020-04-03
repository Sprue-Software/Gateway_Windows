################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include xip/subdir.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include mbedtls/port/ksdk/subdir.mk
-include mbedtls/library/subdir.mk
-include lwip_httpscli_mbedTLS/subdir.mk
-include lwip/src/netif/ppp/subdir.mk
-include lwip/src/netif/subdir.mk
-include lwip/src/core/ipv6/subdir.mk
-include lwip/src/core/ipv4/subdir.mk
-include lwip/src/core/subdir.mk
-include lwip/src/api/subdir.mk
-include lwip/port/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/lists/subdir.mk
-include board/subdir.mk
-include amazon-freertos/freertos/portable/subdir.mk
-include amazon-freertos/freertos/subdir.mk
-include WF200/wfx_fmac_driver/secure_link/subdir.mk
-include WF200/wfx_fmac_driver/bus/subdir.mk
-include WF200/wfx_fmac_driver/subdir.mk
-include WF200/app/tcpecho_raw/subdir.mk
-include WF200/subdir.mk
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
all: WF200_RefProjectToPort.axf

# Tool invocations
WF200_RefProjectToPort.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="WF200_RefProjectToPort.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -T evkbimxrt1050_lwip_httpscli_mbedTLS_freertos_Debug.ld -o "WF200_RefProjectToPort.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) WF200_RefProjectToPort.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "WF200_RefProjectToPort.axf"; # arm-none-eabi-objcopy -v -O binary "WF200_RefProjectToPort.axf" "WF200_RefProjectToPort.bin" ; # checksum -p MIMXRT1052xxxxB -d "WF200_RefProjectToPort.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
