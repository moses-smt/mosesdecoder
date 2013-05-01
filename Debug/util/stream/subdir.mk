################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../util/stream/chain.cc \
../util/stream/io.cc \
../util/stream/io_test.cc \
../util/stream/line_input.cc \
../util/stream/multi_progress.cc \
../util/stream/sort_test.cc \
../util/stream/stream_test.cc 

OBJS += \
./util/stream/chain.o \
./util/stream/io.o \
./util/stream/io_test.o \
./util/stream/line_input.o \
./util/stream/multi_progress.o \
./util/stream/sort_test.o \
./util/stream/stream_test.o 

CC_DEPS += \
./util/stream/chain.d \
./util/stream/io.d \
./util/stream/io_test.d \
./util/stream/line_input.d \
./util/stream/multi_progress.d \
./util/stream/sort_test.d \
./util/stream/stream_test.d 


# Each subdirectory must supply rules for building sources it contributes
util/stream/%.o: ../util/stream/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


