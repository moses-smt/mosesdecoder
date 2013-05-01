################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../phrase-extract/pcfg-score/main.cc \
../phrase-extract/pcfg-score/pcfg_score.cc \
../phrase-extract/pcfg-score/tree_scorer.cc 

OBJS += \
./phrase-extract/pcfg-score/main.o \
./phrase-extract/pcfg-score/pcfg_score.o \
./phrase-extract/pcfg-score/tree_scorer.o 

CC_DEPS += \
./phrase-extract/pcfg-score/main.d \
./phrase-extract/pcfg-score/pcfg_score.d \
./phrase-extract/pcfg-score/tree_scorer.d 


# Each subdirectory must supply rules for building sources it contributes
phrase-extract/pcfg-score/%.o: ../phrase-extract/pcfg-score/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


