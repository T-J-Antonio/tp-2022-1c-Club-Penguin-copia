################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Memoria.c \
../utilsMemoria.c 

OBJS += \
./Memoria.o \
./utilsMemoria.o 

C_DEPS += \
./Memoria.d \
./utilsMemoria.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<" -m32
	@echo 'Finished building: $<'
	@echo ' '


