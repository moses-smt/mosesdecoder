################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../contrib/checkplf/checkplf.cpp 

OBJS += \
./contrib/checkplf/checkplf.o 

CPP_DEPS += \
./contrib/checkplf/checkplf.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/checkplf/%.o: ../contrib/checkplf/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


