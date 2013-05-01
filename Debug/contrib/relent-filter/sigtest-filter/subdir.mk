################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../contrib/relent-filter/sigtest-filter/WIN32_functions.cpp \
../contrib/relent-filter/sigtest-filter/filter-pt.cpp 

OBJS += \
./contrib/relent-filter/sigtest-filter/WIN32_functions.o \
./contrib/relent-filter/sigtest-filter/filter-pt.o 

CPP_DEPS += \
./contrib/relent-filter/sigtest-filter/WIN32_functions.d \
./contrib/relent-filter/sigtest-filter/filter-pt.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/relent-filter/sigtest-filter/%.o: ../contrib/relent-filter/sigtest-filter/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


