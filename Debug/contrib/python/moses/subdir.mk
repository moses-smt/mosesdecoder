################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../contrib/python/moses/dictree.cpp 

OBJS += \
./contrib/python/moses/dictree.o 

CPP_DEPS += \
./contrib/python/moses/dictree.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/python/moses/%.o: ../contrib/python/moses/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


