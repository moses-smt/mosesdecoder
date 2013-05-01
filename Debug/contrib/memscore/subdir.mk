################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../contrib/memscore/lexdecom.cpp \
../contrib/memscore/memscore.cpp \
../contrib/memscore/phraselm.cpp \
../contrib/memscore/phrasetable.cpp \
../contrib/memscore/scorer.cpp 

OBJS += \
./contrib/memscore/lexdecom.o \
./contrib/memscore/memscore.o \
./contrib/memscore/phraselm.o \
./contrib/memscore/phrasetable.o \
./contrib/memscore/scorer.o 

CPP_DEPS += \
./contrib/memscore/lexdecom.d \
./contrib/memscore/memscore.d \
./contrib/memscore/phraselm.d \
./contrib/memscore/phrasetable.d \
./contrib/memscore/scorer.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/memscore/%.o: ../contrib/memscore/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


