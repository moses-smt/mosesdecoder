################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../lm/builder/adjust_counts.cc \
../lm/builder/adjust_counts_test.cc \
../lm/builder/corpus_count.cc \
../lm/builder/corpus_count_test.cc \
../lm/builder/initial_probabilities.cc \
../lm/builder/interpolate.cc \
../lm/builder/lmplz_main.cc \
../lm/builder/pipeline.cc \
../lm/builder/print.cc 

OBJS += \
./lm/builder/adjust_counts.o \
./lm/builder/adjust_counts_test.o \
./lm/builder/corpus_count.o \
./lm/builder/corpus_count_test.o \
./lm/builder/initial_probabilities.o \
./lm/builder/interpolate.o \
./lm/builder/lmplz_main.o \
./lm/builder/pipeline.o \
./lm/builder/print.o 

CC_DEPS += \
./lm/builder/adjust_counts.d \
./lm/builder/adjust_counts_test.d \
./lm/builder/corpus_count.d \
./lm/builder/corpus_count_test.d \
./lm/builder/initial_probabilities.d \
./lm/builder/interpolate.d \
./lm/builder/lmplz_main.d \
./lm/builder/pipeline.d \
./lm/builder/print.d 


# Each subdirectory must supply rules for building sources it contributes
lm/builder/%.o: ../lm/builder/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


