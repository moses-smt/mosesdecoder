################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../contrib/eppex/counter.cpp \
../contrib/eppex/eppex.cpp \
../contrib/eppex/phrase-extract.cpp \
../contrib/eppex/shared.cpp 

OBJS += \
./contrib/eppex/counter.o \
./contrib/eppex/eppex.o \
./contrib/eppex/phrase-extract.o \
./contrib/eppex/shared.o 

CPP_DEPS += \
./contrib/eppex/counter.d \
./contrib/eppex/eppex.d \
./contrib/eppex/phrase-extract.d \
./contrib/eppex/shared.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/eppex/%.o: ../contrib/eppex/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


