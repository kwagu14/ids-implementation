################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32L5xx_HAL_Driver/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := My_Mem_to_Mem_Example_NonSecure
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
My_Mem_to_Mem_Example_NonSecure.elf \

MAP_FILES += \
My_Mem_to_Mem_Example_NonSecure.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
My_Mem_to_Mem_Example_NonSecure.list \


# All Target
all: main-build

# Main-build Target
main-build: My_Mem_to_Mem_Example_NonSecure.elf secondary-outputs

dependents:
	-cd C:/Users/karle/STM32CubeIDE/workspace_1.12.1/My_Mem_to_Mem_Example/Secure/Debug && $(MAKE) clean all

# Tool invocations
My_Mem_to_Mem_Example_NonSecure.elf My_Mem_to_Mem_Example_NonSecure.map: $(OBJS) $(USER_OBJS) C:/Users/karle/STM32CubeIDE/workspace_1.12.1/My_Mem_to_Mem_Example/Secure/Debug/My_Mem_to_Mem_Example_Secure.elf C:\Users\karle\STM32CubeIDE\workspace_1.12.1\My_Mem_to_Mem_Example\NonSecure\STM32L562QEIXQ_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "My_Mem_to_Mem_Example_NonSecure.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m33 -T"C:\Users\karle\STM32CubeIDE\workspace_1.12.1\My_Mem_to_Mem_Example\NonSecure\STM32L562QEIXQ_FLASH.ld" --specs=nosys.specs -Wl,-Map="My_Mem_to_Mem_Example_NonSecure.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

My_Mem_to_Mem_Example_NonSecure.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "My_Mem_to_Mem_Example_NonSecure.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) My_Mem_to_Mem_Example_NonSecure.elf My_Mem_to_Mem_Example_NonSecure.list My_Mem_to_Mem_Example_NonSecure.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified
C:/Users/karle/STM32CubeIDE/workspace_1.12.1/My_Mem_to_Mem_Example/Secure/Debug/My_Mem_to_Mem_Example_Secure.elf:

-include ../makefile.targets
