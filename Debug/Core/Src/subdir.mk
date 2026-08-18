################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/elink_cmd.c \
../Core/Src/freertos.c \
../Core/Src/ina219.c \
../Core/Src/main.c \
../Core/Src/max31865.c \
../Core/Src/minmea.c \
../Core/Src/ms5803_01ba.c \
../Core/Src/npa730n015a.c \
../Core/Src/sensors.c \
../Core/Src/sht4x.c \
../Core/Src/stm32l4xx_hal_msp.c \
../Core/Src/stm32l4xx_hal_timebase_tim.c \
../Core/Src/stm32l4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32l4xx.c \
../Core/Src/telemetry.c \
../Core/Src/tmp102.c \
../Core/Src/txhalow.c \
../Core/Src/w25log.c \
../Core/Src/w25qxx.c 

OBJS += \
./Core/Src/elink_cmd.o \
./Core/Src/freertos.o \
./Core/Src/ina219.o \
./Core/Src/main.o \
./Core/Src/max31865.o \
./Core/Src/minmea.o \
./Core/Src/ms5803_01ba.o \
./Core/Src/npa730n015a.o \
./Core/Src/sensors.o \
./Core/Src/sht4x.o \
./Core/Src/stm32l4xx_hal_msp.o \
./Core/Src/stm32l4xx_hal_timebase_tim.o \
./Core/Src/stm32l4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32l4xx.o \
./Core/Src/telemetry.o \
./Core/Src/tmp102.o \
./Core/Src/txhalow.o \
./Core/Src/w25log.o \
./Core/Src/w25qxx.o 

C_DEPS += \
./Core/Src/elink_cmd.d \
./Core/Src/freertos.d \
./Core/Src/ina219.d \
./Core/Src/main.d \
./Core/Src/max31865.d \
./Core/Src/minmea.d \
./Core/Src/ms5803_01ba.d \
./Core/Src/npa730n015a.d \
./Core/Src/sensors.d \
./Core/Src/sht4x.d \
./Core/Src/stm32l4xx_hal_msp.d \
./Core/Src/stm32l4xx_hal_timebase_tim.d \
./Core/Src/stm32l4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32l4xx.d \
./Core/Src/telemetry.d \
./Core/Src/tmp102.d \
./Core/Src/txhalow.d \
./Core/Src/w25log.d \
./Core/Src/w25qxx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I../Core/Inc -I"C:/Users/Mario/Documents/BLESS/BLESS-Source-Code/Drivers/Ethernet_W5500" -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/elink_cmd.cyclo ./Core/Src/elink_cmd.d ./Core/Src/elink_cmd.o ./Core/Src/elink_cmd.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/ina219.cyclo ./Core/Src/ina219.d ./Core/Src/ina219.o ./Core/Src/ina219.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/max31865.cyclo ./Core/Src/max31865.d ./Core/Src/max31865.o ./Core/Src/max31865.su ./Core/Src/minmea.cyclo ./Core/Src/minmea.d ./Core/Src/minmea.o ./Core/Src/minmea.su ./Core/Src/ms5803_01ba.cyclo ./Core/Src/ms5803_01ba.d ./Core/Src/ms5803_01ba.o ./Core/Src/ms5803_01ba.su ./Core/Src/npa730n015a.cyclo ./Core/Src/npa730n015a.d ./Core/Src/npa730n015a.o ./Core/Src/npa730n015a.su ./Core/Src/sensors.cyclo ./Core/Src/sensors.d ./Core/Src/sensors.o ./Core/Src/sensors.su ./Core/Src/sht4x.cyclo ./Core/Src/sht4x.d ./Core/Src/sht4x.o ./Core/Src/sht4x.su ./Core/Src/stm32l4xx_hal_msp.cyclo ./Core/Src/stm32l4xx_hal_msp.d ./Core/Src/stm32l4xx_hal_msp.o ./Core/Src/stm32l4xx_hal_msp.su ./Core/Src/stm32l4xx_hal_timebase_tim.cyclo ./Core/Src/stm32l4xx_hal_timebase_tim.d ./Core/Src/stm32l4xx_hal_timebase_tim.o ./Core/Src/stm32l4xx_hal_timebase_tim.su ./Core/Src/stm32l4xx_it.cyclo ./Core/Src/stm32l4xx_it.d ./Core/Src/stm32l4xx_it.o ./Core/Src/stm32l4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32l4xx.cyclo ./Core/Src/system_stm32l4xx.d ./Core/Src/system_stm32l4xx.o ./Core/Src/system_stm32l4xx.su ./Core/Src/telemetry.cyclo ./Core/Src/telemetry.d ./Core/Src/telemetry.o ./Core/Src/telemetry.su ./Core/Src/tmp102.cyclo ./Core/Src/tmp102.d ./Core/Src/tmp102.o ./Core/Src/tmp102.su ./Core/Src/txhalow.cyclo ./Core/Src/txhalow.d ./Core/Src/txhalow.o ./Core/Src/txhalow.su ./Core/Src/w25log.cyclo ./Core/Src/w25log.d ./Core/Src/w25log.o ./Core/Src/w25log.su ./Core/Src/w25qxx.cyclo ./Core/Src/w25qxx.d ./Core/Src/w25qxx.o ./Core/Src/w25qxx.su

.PHONY: clean-Core-2f-Src

