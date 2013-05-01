################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../util/double-conversion/bignum-dtoa.cc \
../util/double-conversion/bignum.cc \
../util/double-conversion/cached-powers.cc \
../util/double-conversion/diy-fp.cc \
../util/double-conversion/double-conversion.cc \
../util/double-conversion/fast-dtoa.cc \
../util/double-conversion/fixed-dtoa.cc \
../util/double-conversion/strtod.cc 

OBJS += \
./util/double-conversion/bignum-dtoa.o \
./util/double-conversion/bignum.o \
./util/double-conversion/cached-powers.o \
./util/double-conversion/diy-fp.o \
./util/double-conversion/double-conversion.o \
./util/double-conversion/fast-dtoa.o \
./util/double-conversion/fixed-dtoa.o \
./util/double-conversion/strtod.o 

CC_DEPS += \
./util/double-conversion/bignum-dtoa.d \
./util/double-conversion/bignum.d \
./util/double-conversion/cached-powers.d \
./util/double-conversion/diy-fp.d \
./util/double-conversion/double-conversion.d \
./util/double-conversion/fast-dtoa.d \
./util/double-conversion/fixed-dtoa.d \
./util/double-conversion/strtod.d 


# Each subdirectory must supply rules for building sources it contributes
util/double-conversion/%.o: ../util/double-conversion/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


