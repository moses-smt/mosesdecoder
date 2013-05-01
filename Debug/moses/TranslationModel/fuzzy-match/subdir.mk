################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses/TranslationModel/fuzzy-match/Alignments.cpp \
../moses/TranslationModel/fuzzy-match/FuzzyMatchWrapper.cpp \
../moses/TranslationModel/fuzzy-match/SentenceAlignment.cpp \
../moses/TranslationModel/fuzzy-match/SuffixArray.cpp \
../moses/TranslationModel/fuzzy-match/Vocabulary.cpp \
../moses/TranslationModel/fuzzy-match/create_xml.cpp 

OBJS += \
./moses/TranslationModel/fuzzy-match/Alignments.o \
./moses/TranslationModel/fuzzy-match/FuzzyMatchWrapper.o \
./moses/TranslationModel/fuzzy-match/SentenceAlignment.o \
./moses/TranslationModel/fuzzy-match/SuffixArray.o \
./moses/TranslationModel/fuzzy-match/Vocabulary.o \
./moses/TranslationModel/fuzzy-match/create_xml.o 

CPP_DEPS += \
./moses/TranslationModel/fuzzy-match/Alignments.d \
./moses/TranslationModel/fuzzy-match/FuzzyMatchWrapper.d \
./moses/TranslationModel/fuzzy-match/SentenceAlignment.d \
./moses/TranslationModel/fuzzy-match/SuffixArray.d \
./moses/TranslationModel/fuzzy-match/Vocabulary.d \
./moses/TranslationModel/fuzzy-match/create_xml.d 


# Each subdirectory must supply rules for building sources it contributes
moses/TranslationModel/fuzzy-match/%.o: ../moses/TranslationModel/fuzzy-match/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


