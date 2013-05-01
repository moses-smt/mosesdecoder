################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../phrase-extract/pcfg-common/pcfg.cc \
../phrase-extract/pcfg-common/tool.cc \
../phrase-extract/pcfg-common/xml_tree_parser.cc 

OBJS += \
./phrase-extract/pcfg-common/pcfg.o \
./phrase-extract/pcfg-common/tool.o \
./phrase-extract/pcfg-common/xml_tree_parser.o 

CC_DEPS += \
./phrase-extract/pcfg-common/pcfg.d \
./phrase-extract/pcfg-common/tool.d \
./phrase-extract/pcfg-common/xml_tree_parser.d 


# Each subdirectory must supply rules for building sources it contributes
phrase-extract/pcfg-common/%.o: ../phrase-extract/pcfg-common/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


