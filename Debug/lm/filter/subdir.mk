################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../lm/filter/arpa_io.cc \
../lm/filter/filter_main.cc \
../lm/filter/phrase.cc \
../lm/filter/vocab.cc 

OBJS += \
./lm/filter/arpa_io.o \
./lm/filter/filter_main.o \
./lm/filter/phrase.o \
./lm/filter/vocab.o 

CC_DEPS += \
./lm/filter/arpa_io.d \
./lm/filter/filter_main.d \
./lm/filter/phrase.d \
./lm/filter/vocab.d 


# Each subdirectory must supply rules for building sources it contributes
lm/filter/%.o: ../lm/filter/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


