################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../mira/Decoder.cpp \
../mira/Hildreth.cpp \
../mira/HildrethTest.cpp \
../mira/HypothesisQueue.cpp \
../mira/Main.cpp \
../mira/MiraOptimiser.cpp \
../mira/MiraTest.cpp \
../mira/Perceptron.cpp 

OBJS += \
./mira/Decoder.o \
./mira/Hildreth.o \
./mira/HildrethTest.o \
./mira/HypothesisQueue.o \
./mira/Main.o \
./mira/MiraOptimiser.o \
./mira/MiraTest.o \
./mira/Perceptron.o 

CPP_DEPS += \
./mira/Decoder.d \
./mira/Hildreth.d \
./mira/HildrethTest.d \
./mira/HypothesisQueue.d \
./mira/Main.d \
./mira/MiraOptimiser.d \
./mira/MiraTest.d \
./mira/Perceptron.d 


# Each subdirectory must supply rules for building sources it contributes
mira/%.o: ../mira/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


