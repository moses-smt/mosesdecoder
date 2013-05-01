################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../misc/pmoses/pmoses.cc 

OBJS += \
./misc/pmoses/pmoses.o 

CC_DEPS += \
./misc/pmoses/pmoses.d 


# Each subdirectory must supply rules for building sources it contributes
misc/pmoses/%.o: ../misc/pmoses/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


