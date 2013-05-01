################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses/TranslationModel/CompactPT/BlockHashIndex.cpp \
../moses/TranslationModel/CompactPT/CmphStringVectorAdapter.cpp \
../moses/TranslationModel/CompactPT/LexicalReorderingTableCompact.cpp \
../moses/TranslationModel/CompactPT/LexicalReorderingTableCreator.cpp \
../moses/TranslationModel/CompactPT/MurmurHash3.cpp \
../moses/TranslationModel/CompactPT/PhraseDecoder.cpp \
../moses/TranslationModel/CompactPT/PhraseDictionaryCompact.cpp \
../moses/TranslationModel/CompactPT/PhraseTableCreator.cpp \
../moses/TranslationModel/CompactPT/ThrowingFwrite.cpp 

OBJS += \
./moses/TranslationModel/CompactPT/BlockHashIndex.o \
./moses/TranslationModel/CompactPT/CmphStringVectorAdapter.o \
./moses/TranslationModel/CompactPT/LexicalReorderingTableCompact.o \
./moses/TranslationModel/CompactPT/LexicalReorderingTableCreator.o \
./moses/TranslationModel/CompactPT/MurmurHash3.o \
./moses/TranslationModel/CompactPT/PhraseDecoder.o \
./moses/TranslationModel/CompactPT/PhraseDictionaryCompact.o \
./moses/TranslationModel/CompactPT/PhraseTableCreator.o \
./moses/TranslationModel/CompactPT/ThrowingFwrite.o 

CPP_DEPS += \
./moses/TranslationModel/CompactPT/BlockHashIndex.d \
./moses/TranslationModel/CompactPT/CmphStringVectorAdapter.d \
./moses/TranslationModel/CompactPT/LexicalReorderingTableCompact.d \
./moses/TranslationModel/CompactPT/LexicalReorderingTableCreator.d \
./moses/TranslationModel/CompactPT/MurmurHash3.d \
./moses/TranslationModel/CompactPT/PhraseDecoder.d \
./moses/TranslationModel/CompactPT/PhraseDictionaryCompact.d \
./moses/TranslationModel/CompactPT/PhraseTableCreator.d \
./moses/TranslationModel/CompactPT/ThrowingFwrite.d 


# Each subdirectory must supply rules for building sources it contributes
moses/TranslationModel/CompactPT/%.o: ../moses/TranslationModel/CompactPT/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


