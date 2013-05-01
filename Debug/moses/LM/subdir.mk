################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses/LM/Base.cpp \
../moses/LM/Factory.cpp \
../moses/LM/IRST.cpp \
../moses/LM/Implementation.cpp \
../moses/LM/Joint.cpp \
../moses/LM/Ken.cpp \
../moses/LM/LDHT.cpp \
../moses/LM/MultiFactor.cpp \
../moses/LM/ORLM.cpp \
../moses/LM/ParallelBackoff.cpp \
../moses/LM/Rand.cpp \
../moses/LM/Remote.cpp \
../moses/LM/SRI.cpp \
../moses/LM/SingleFactor.cpp 

OBJS += \
./moses/LM/Base.o \
./moses/LM/Factory.o \
./moses/LM/IRST.o \
./moses/LM/Implementation.o \
./moses/LM/Joint.o \
./moses/LM/Ken.o \
./moses/LM/LDHT.o \
./moses/LM/MultiFactor.o \
./moses/LM/ORLM.o \
./moses/LM/ParallelBackoff.o \
./moses/LM/Rand.o \
./moses/LM/Remote.o \
./moses/LM/SRI.o \
./moses/LM/SingleFactor.o 

CPP_DEPS += \
./moses/LM/Base.d \
./moses/LM/Factory.d \
./moses/LM/IRST.d \
./moses/LM/Implementation.d \
./moses/LM/Joint.d \
./moses/LM/Ken.d \
./moses/LM/LDHT.d \
./moses/LM/MultiFactor.d \
./moses/LM/ORLM.d \
./moses/LM/ParallelBackoff.d \
./moses/LM/Rand.d \
./moses/LM/Remote.d \
./moses/LM/SRI.d \
./moses/LM/SingleFactor.d 


# Each subdirectory must supply rules for building sources it contributes
moses/LM/%.o: ../moses/LM/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


