################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/karle/STM32CubeIDE/workspace_1.12.1/UART_Printf/Drivers/BSP/STM32L552E-EV/stm32l552e_eval.c 

OBJS += \
./Drivers/BSP/STM32L552E-EV/stm32l552e_eval.o 

C_DEPS += \
./Drivers/BSP/STM32L552E-EV/stm32l552e_eval.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32L552E-EV/stm32l552e_eval.o: C:/Users/karle/STM32CubeIDE/workspace_1.12.1/UART_Printf/Drivers/BSP/STM32L552E-EV/stm32l552e_eval.c Drivers/BSP/STM32L552E-EV/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG -c -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Inc -I../../Drivers/BSP/STM32L552E-EV -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-STM32L552E-2d-EV

clean-Drivers-2f-BSP-2f-STM32L552E-2d-EV:
	-$(RM) ./Drivers/BSP/STM32L552E-EV/stm32l552e_eval.cyclo ./Drivers/BSP/STM32L552E-EV/stm32l552e_eval.d ./Drivers/BSP/STM32L552E-EV/stm32l552e_eval.o ./Drivers/BSP/STM32L552E-EV/stm32l552e_eval.su

.PHONY: clean-Drivers-2f-BSP-2f-STM32L552E-2d-EV

