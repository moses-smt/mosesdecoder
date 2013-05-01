################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses/TranslationModel/BilingualDynSuffixArray.cpp \
../moses/TranslationModel/DynSuffixArray.cpp \
../moses/TranslationModel/PhraseDictionary.cpp \
../moses/TranslationModel/PhraseDictionaryCache.cpp \
../moses/TranslationModel/PhraseDictionaryDynSuffixArray.cpp \
../moses/TranslationModel/PhraseDictionaryMemory.cpp \
../moses/TranslationModel/PhraseDictionaryNode.cpp \
../moses/TranslationModel/PhraseDictionaryTree.cpp \
../moses/TranslationModel/PhraseDictionaryTreeAdaptor.cpp 

OBJS += \
./moses/TranslationModel/BilingualDynSuffixArray.o \
./moses/TranslationModel/DynSuffixArray.o \
./moses/TranslationModel/PhraseDictionary.o \
./moses/TranslationModel/PhraseDictionaryCache.o \
./moses/TranslationModel/PhraseDictionaryDynSuffixArray.o \
./moses/TranslationModel/PhraseDictionaryMemory.o \
./moses/TranslationModel/PhraseDictionaryNode.o \
./moses/TranslationModel/PhraseDictionaryTree.o \
./moses/TranslationModel/PhraseDictionaryTreeAdaptor.o 

CPP_DEPS += \
./moses/TranslationModel/BilingualDynSuffixArray.d \
./moses/TranslationModel/DynSuffixArray.d \
./moses/TranslationModel/PhraseDictionary.d \
./moses/TranslationModel/PhraseDictionaryCache.d \
./moses/TranslationModel/PhraseDictionaryDynSuffixArray.d \
./moses/TranslationModel/PhraseDictionaryMemory.d \
./moses/TranslationModel/PhraseDictionaryNode.d \
./moses/TranslationModel/PhraseDictionaryTree.d \
./moses/TranslationModel/PhraseDictionaryTreeAdaptor.d 


# Each subdirectory must supply rules for building sources it contributes
moses/TranslationModel/%.o: ../moses/TranslationModel/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


