################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../contrib/server/mosesserver.cpp 

OBJS += \
./contrib/server/mosesserver.o 

CPP_DEPS += \
./contrib/server/mosesserver.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/server/%.o: ../contrib/server/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


