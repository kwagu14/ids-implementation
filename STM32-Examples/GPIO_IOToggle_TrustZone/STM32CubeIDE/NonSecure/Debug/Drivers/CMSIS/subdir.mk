################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/karle/STM32CubeIDE/workspace_1.12.1/GPIO_IOToggle_TrustZone/NonSecure/Src/system_stm32l5xx_ns.c 

OBJS += \
./Drivers/CMSIS/system_stm32l5xx_ns.o 

C_DEPS += \
./Drivers/CMSIS/system_stm32l5xx_ns.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/CMSIS/system_stm32l5xx_ns.o: C:/Users/karle/STM32CubeIDE/workspace_1.12.1/GPIO_IOToggle_TrustZone/NonSecure/Src/system_stm32l5xx_ns.c Drivers/CMSIS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L562xx -DDEBUG -c -I../../../NonSecure/Inc -I../../../Secure_nsclib -I../../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-CMSIS

clean-Drivers-2f-CMSIS:
	-$(RM) ./Drivers/CMSIS/system_stm32l5xx_ns.cyclo ./Drivers/CMSIS/system_stm32l5xx_ns.d ./Drivers/CMSIS/system_stm32l5xx_ns.o ./Drivers/CMSIS/system_stm32l5xx_ns.su

.PHONY: clean-Drivers-2f-CMSIS

