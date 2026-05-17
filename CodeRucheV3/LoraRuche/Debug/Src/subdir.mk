################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/gpio.c \
../Src/interrupt.c \
../Src/lora.c \
../Src/main.c \
../Src/onewire.c \
../Src/poids.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/timer.c \
../Src/usart.c 

OBJS += \
./Src/gpio.o \
./Src/interrupt.o \
./Src/lora.o \
./Src/main.o \
./Src/onewire.o \
./Src/poids.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/timer.o \
./Src/usart.o 

C_DEPS += \
./Src/gpio.d \
./Src/interrupt.d \
./Src/lora.d \
./Src/main.d \
./Src/onewire.d \
./Src/poids.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/timer.d \
./Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32G0 -DSTM32G031K8Tx -c -I../Inc -I"D:/Users/user/Downloads/LoraRucheV2/LoraRuche2/Drivers" -I"D:/Users/user/Downloads/LoraRucheV2/LoraRuche2/Src" -I"D:/Users/user/Downloads/LoraRucheV2/LoraRuche2/Drivers/CMSIS/Device/ST/STM32G0xx/Include" -I"D:/Users/user/Downloads/LoraRucheV2/LoraRuche2/Drivers/CMSIS/Include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/gpio.cyclo ./Src/gpio.d ./Src/gpio.o ./Src/gpio.su ./Src/interrupt.cyclo ./Src/interrupt.d ./Src/interrupt.o ./Src/interrupt.su ./Src/lora.cyclo ./Src/lora.d ./Src/lora.o ./Src/lora.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/onewire.cyclo ./Src/onewire.d ./Src/onewire.o ./Src/onewire.su ./Src/poids.cyclo ./Src/poids.d ./Src/poids.o ./Src/poids.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/timer.cyclo ./Src/timer.d ./Src/timer.o ./Src/timer.su ./Src/usart.cyclo ./Src/usart.d ./Src/usart.o ./Src/usart.su

.PHONY: clean-Src

