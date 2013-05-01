################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../phrase-extract/lexical-reordering/InputFileStream.cpp \
../phrase-extract/lexical-reordering/reordering_classes.cpp \
../phrase-extract/lexical-reordering/score.cpp 

OBJS += \
./phrase-extract/lexical-reordering/InputFileStream.o \
./phrase-extract/lexical-reordering/reordering_classes.o \
./phrase-extract/lexical-reordering/score.o 

CPP_DEPS += \
./phrase-extract/lexical-reordering/InputFileStream.d \
./phrase-extract/lexical-reordering/reordering_classes.d \
./phrase-extract/lexical-reordering/score.d 


# Each subdirectory must supply rules for building sources it contributes
phrase-extract/lexical-reordering/%.o: ../phrase-extract/lexical-reordering/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


