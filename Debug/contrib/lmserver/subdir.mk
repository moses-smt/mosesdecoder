################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../contrib/lmserver/daemon.c \
../contrib/lmserver/lmserver.c \
../contrib/lmserver/thread.c 

CC_SRCS += \
../contrib/lmserver/srilm.cc 

OBJS += \
./contrib/lmserver/daemon.o \
./contrib/lmserver/lmserver.o \
./contrib/lmserver/srilm.o \
./contrib/lmserver/thread.o 

C_DEPS += \
./contrib/lmserver/daemon.d \
./contrib/lmserver/lmserver.d \
./contrib/lmserver/thread.d 

CC_DEPS += \
./contrib/lmserver/srilm.d 


# Each subdirectory must supply rules for building sources it contributes
contrib/lmserver/%.o: ../contrib/lmserver/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

contrib/lmserver/%.o: ../contrib/lmserver/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


