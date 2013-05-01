################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses-cmd/IOWrapper.cpp \
../moses-cmd/LatticeMBR.cpp \
../moses-cmd/LatticeMBRGrid.cpp \
../moses-cmd/Main.cpp \
../moses-cmd/TranslationAnalysis.cpp \
../moses-cmd/mbr.cpp 

OBJS += \
./moses-cmd/IOWrapper.o \
./moses-cmd/LatticeMBR.o \
./moses-cmd/LatticeMBRGrid.o \
./moses-cmd/Main.o \
./moses-cmd/TranslationAnalysis.o \
./moses-cmd/mbr.o 

CPP_DEPS += \
./moses-cmd/IOWrapper.d \
./moses-cmd/LatticeMBR.d \
./moses-cmd/LatticeMBRGrid.d \
./moses-cmd/Main.d \
./moses-cmd/TranslationAnalysis.d \
./moses-cmd/mbr.d 


# Each subdirectory must supply rules for building sources it contributes
moses-cmd/%.o: ../moses-cmd/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


