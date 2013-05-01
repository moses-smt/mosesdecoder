################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../contrib/sigtest-filter/WIN32_functions.cpp \
../contrib/sigtest-filter/filter-pt.cpp 

OBJS += \
./contrib/sigtest-filter/WIN32_functions.o \
./contrib/sigtest-filter/filter-pt.o 

CPP_DEPS += \
./contrib/sigtest-filter/WIN32_functions.d \
./contrib/sigtest-filter/filter-pt.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/sigtest-filter/%.o: ../contrib/sigtest-filter/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


