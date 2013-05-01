################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../OnDiskPt/Main.cpp \
../OnDiskPt/OnDiskQuery.cpp \
../OnDiskPt/OnDiskWrapper.cpp \
../OnDiskPt/Phrase.cpp \
../OnDiskPt/PhraseNode.cpp \
../OnDiskPt/SourcePhrase.cpp \
../OnDiskPt/TargetPhrase.cpp \
../OnDiskPt/TargetPhraseCollection.cpp \
../OnDiskPt/Vocab.cpp \
../OnDiskPt/Word.cpp \
../OnDiskPt/queryOnDiskPt.cpp 

OBJS += \
./OnDiskPt/Main.o \
./OnDiskPt/OnDiskQuery.o \
./OnDiskPt/OnDiskWrapper.o \
./OnDiskPt/Phrase.o \
./OnDiskPt/PhraseNode.o \
./OnDiskPt/SourcePhrase.o \
./OnDiskPt/TargetPhrase.o \
./OnDiskPt/TargetPhraseCollection.o \
./OnDiskPt/Vocab.o \
./OnDiskPt/Word.o \
./OnDiskPt/queryOnDiskPt.o 

CPP_DEPS += \
./OnDiskPt/Main.d \
./OnDiskPt/OnDiskQuery.d \
./OnDiskPt/OnDiskWrapper.d \
./OnDiskPt/Phrase.d \
./OnDiskPt/PhraseNode.d \
./OnDiskPt/SourcePhrase.d \
./OnDiskPt/TargetPhrase.d \
./OnDiskPt/TargetPhraseCollection.d \
./OnDiskPt/Vocab.d \
./OnDiskPt/Word.d \
./OnDiskPt/queryOnDiskPt.d 


# Each subdirectory must supply rules for building sources it contributes
OnDiskPt/%.o: ../OnDiskPt/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


