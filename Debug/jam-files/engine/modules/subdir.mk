################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../jam-files/engine/modules/order.c \
../jam-files/engine/modules/path.c \
../jam-files/engine/modules/property-set.c \
../jam-files/engine/modules/regex.c \
../jam-files/engine/modules/sequence.c \
../jam-files/engine/modules/set.c 

OBJS += \
./jam-files/engine/modules/order.o \
./jam-files/engine/modules/path.o \
./jam-files/engine/modules/property-set.o \
./jam-files/engine/modules/regex.o \
./jam-files/engine/modules/sequence.o \
./jam-files/engine/modules/set.o 

C_DEPS += \
./jam-files/engine/modules/order.d \
./jam-files/engine/modules/path.d \
./jam-files/engine/modules/property-set.d \
./jam-files/engine/modules/regex.d \
./jam-files/engine/modules/sequence.d \
./jam-files/engine/modules/set.d 


# Each subdirectory must supply rules for building sources it contributes
jam-files/engine/modules/%.o: ../jam-files/engine/modules/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


