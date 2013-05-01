################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../misc/GenerateTuples.cpp \
../misc/processLexicalTable.cpp \
../misc/processLexicalTableMin.cpp \
../misc/processPhraseTable.cpp \
../misc/processPhraseTableMin.cpp \
../misc/queryLexicalTable.cpp \
../misc/queryPhraseTable.cpp \
../misc/queryPhraseTableMin.cpp 

OBJS += \
./misc/GenerateTuples.o \
./misc/processLexicalTable.o \
./misc/processLexicalTableMin.o \
./misc/processPhraseTable.o \
./misc/processPhraseTableMin.o \
./misc/queryLexicalTable.o \
./misc/queryPhraseTable.o \
./misc/queryPhraseTableMin.o 

CPP_DEPS += \
./misc/GenerateTuples.d \
./misc/processLexicalTable.d \
./misc/processLexicalTableMin.d \
./misc/processPhraseTable.d \
./misc/processPhraseTableMin.d \
./misc/queryLexicalTable.d \
./misc/queryPhraseTable.d \
./misc/queryPhraseTableMin.d 


# Each subdirectory must supply rules for building sources it contributes
misc/%.o: ../misc/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


