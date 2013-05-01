################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../phrase-extract/pcfg-extract/main.cc \
../phrase-extract/pcfg-extract/pcfg_extract.cc \
../phrase-extract/pcfg-extract/rule_collection.cc \
../phrase-extract/pcfg-extract/rule_extractor.cc 

OBJS += \
./phrase-extract/pcfg-extract/main.o \
./phrase-extract/pcfg-extract/pcfg_extract.o \
./phrase-extract/pcfg-extract/rule_collection.o \
./phrase-extract/pcfg-extract/rule_extractor.o 

CC_DEPS += \
./phrase-extract/pcfg-extract/main.d \
./phrase-extract/pcfg-extract/pcfg_extract.d \
./phrase-extract/pcfg-extract/rule_collection.d \
./phrase-extract/pcfg-extract/rule_extractor.d 


# Each subdirectory must supply rules for building sources it contributes
phrase-extract/pcfg-extract/%.o: ../phrase-extract/pcfg-extract/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


