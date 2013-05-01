################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../contrib/lmserver/examples/lmclient.cc 

OBJS += \
./contrib/lmserver/examples/lmclient.o 

CC_DEPS += \
./contrib/lmserver/examples/lmclient.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/lmserver/examples/%.o: ../contrib/lmserver/examples/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


