################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../biconcor/Alignment.cpp \
../biconcor/Mismatch.cpp \
../biconcor/PhrasePair.cpp \
../biconcor/PhrasePairCollection.cpp \
../biconcor/SuffixArray.cpp \
../biconcor/TargetCorpus.cpp \
../biconcor/Vocabulary.cpp \
../biconcor/base64.cpp \
../biconcor/biconcor.cpp 

OBJS += \
./biconcor/Alignment.o \
./biconcor/Mismatch.o \
./biconcor/PhrasePair.o \
./biconcor/PhrasePairCollection.o \
./biconcor/SuffixArray.o \
./biconcor/TargetCorpus.o \
./biconcor/Vocabulary.o \
./biconcor/base64.o \
./biconcor/biconcor.o 

CPP_DEPS += \
./biconcor/Alignment.d \
./biconcor/Mismatch.d \
./biconcor/PhrasePair.d \
./biconcor/PhrasePairCollection.d \
./biconcor/SuffixArray.d \
./biconcor/TargetCorpus.d \
./biconcor/Vocabulary.d \
./biconcor/base64.d \
./biconcor/biconcor.d 


# Each subdirectory must supply rules for building sources it contributes
biconcor/%.o: ../biconcor/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


