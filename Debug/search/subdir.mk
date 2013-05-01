################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../search/edge_generator.cc \
../search/nbest.cc \
../search/rule.cc \
../search/vertex.cc 

OBJS += \
./search/edge_generator.o \
./search/nbest.o \
./search/rule.o \
./search/vertex.o 

CC_DEPS += \
./search/edge_generator.d \
./search/nbest.d \
./search/rule.d \
./search/vertex.d 


# Each subdirectory must supply rules for building sources it contributes
search/%.o: ../search/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


