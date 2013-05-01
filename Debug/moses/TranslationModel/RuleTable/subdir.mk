################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses/TranslationModel/RuleTable/LoaderCompact.cpp \
../moses/TranslationModel/RuleTable/LoaderFactory.cpp \
../moses/TranslationModel/RuleTable/LoaderHiero.cpp \
../moses/TranslationModel/RuleTable/LoaderStandard.cpp \
../moses/TranslationModel/RuleTable/PhraseDictionaryALSuffixArray.cpp \
../moses/TranslationModel/RuleTable/PhraseDictionaryFuzzyMatch.cpp \
../moses/TranslationModel/RuleTable/PhraseDictionaryNodeSCFG.cpp \
../moses/TranslationModel/RuleTable/PhraseDictionaryOnDisk.cpp \
../moses/TranslationModel/RuleTable/PhraseDictionarySCFG.cpp \
../moses/TranslationModel/RuleTable/Trie.cpp \
../moses/TranslationModel/RuleTable/UTrie.cpp \
../moses/TranslationModel/RuleTable/UTrieNode.cpp 

OBJS += \
./moses/TranslationModel/RuleTable/LoaderCompact.o \
./moses/TranslationModel/RuleTable/LoaderFactory.o \
./moses/TranslationModel/RuleTable/LoaderHiero.o \
./moses/TranslationModel/RuleTable/LoaderStandard.o \
./moses/TranslationModel/RuleTable/PhraseDictionaryALSuffixArray.o \
./moses/TranslationModel/RuleTable/PhraseDictionaryFuzzyMatch.o \
./moses/TranslationModel/RuleTable/PhraseDictionaryNodeSCFG.o \
./moses/TranslationModel/RuleTable/PhraseDictionaryOnDisk.o \
./moses/TranslationModel/RuleTable/PhraseDictionarySCFG.o \
./moses/TranslationModel/RuleTable/Trie.o \
./moses/TranslationModel/RuleTable/UTrie.o \
./moses/TranslationModel/RuleTable/UTrieNode.o 

CPP_DEPS += \
./moses/TranslationModel/RuleTable/LoaderCompact.d \
./moses/TranslationModel/RuleTable/LoaderFactory.d \
./moses/TranslationModel/RuleTable/LoaderHiero.d \
./moses/TranslationModel/RuleTable/LoaderStandard.d \
./moses/TranslationModel/RuleTable/PhraseDictionaryALSuffixArray.d \
./moses/TranslationModel/RuleTable/PhraseDictionaryFuzzyMatch.d \
./moses/TranslationModel/RuleTable/PhraseDictionaryNodeSCFG.d \
./moses/TranslationModel/RuleTable/PhraseDictionaryOnDisk.d \
./moses/TranslationModel/RuleTable/PhraseDictionarySCFG.d \
./moses/TranslationModel/RuleTable/Trie.d \
./moses/TranslationModel/RuleTable/UTrie.d \
./moses/TranslationModel/RuleTable/UTrieNode.d 


# Each subdirectory must supply rules for building sources it contributes
moses/TranslationModel/RuleTable/%.o: ../moses/TranslationModel/RuleTable/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


