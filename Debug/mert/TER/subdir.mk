################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../mert/TER/alignmentStruct.cpp \
../mert/TER/hashMap.cpp \
../mert/TER/hashMapInfos.cpp \
../mert/TER/hashMapStringInfos.cpp \
../mert/TER/infosHasher.cpp \
../mert/TER/stringHasher.cpp \
../mert/TER/stringInfosHasher.cpp \
../mert/TER/terAlignment.cpp \
../mert/TER/terShift.cpp \
../mert/TER/tercalc.cpp \
../mert/TER/tools.cpp 

OBJS += \
./mert/TER/alignmentStruct.o \
./mert/TER/hashMap.o \
./mert/TER/hashMapInfos.o \
./mert/TER/hashMapStringInfos.o \
./mert/TER/infosHasher.o \
./mert/TER/stringHasher.o \
./mert/TER/stringInfosHasher.o \
./mert/TER/terAlignment.o \
./mert/TER/terShift.o \
./mert/TER/tercalc.o \
./mert/TER/tools.o 

CPP_DEPS += \
./mert/TER/alignmentStruct.d \
./mert/TER/hashMap.d \
./mert/TER/hashMapInfos.d \
./mert/TER/hashMapStringInfos.d \
./mert/TER/infosHasher.d \
./mert/TER/stringHasher.d \
./mert/TER/stringInfosHasher.d \
./mert/TER/terAlignment.d \
./mert/TER/terShift.d \
./mert/TER/tercalc.d \
./mert/TER/tools.d 


# Each subdirectory must supply rules for building sources it contributes
mert/TER/%.o: ../mert/TER/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


