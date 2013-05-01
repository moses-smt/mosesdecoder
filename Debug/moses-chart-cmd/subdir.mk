################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses-chart-cmd/IOWrapper.cpp \
../moses-chart-cmd/Main.cpp \
../moses-chart-cmd/TranslationAnalysis.cpp \
../moses-chart-cmd/mbr.cpp 

OBJS += \
./moses-chart-cmd/IOWrapper.o \
./moses-chart-cmd/Main.o \
./moses-chart-cmd/TranslationAnalysis.o \
./moses-chart-cmd/mbr.o 

CPP_DEPS += \
./moses-chart-cmd/IOWrapper.d \
./moses-chart-cmd/Main.d \
./moses-chart-cmd/TranslationAnalysis.d \
./moses-chart-cmd/mbr.d 


# Each subdirectory must supply rules for building sources it contributes
moses-chart-cmd/%.o: ../moses-chart-cmd/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


