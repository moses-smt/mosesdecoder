################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerCYKPlus.cpp \
../moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemory.cpp \
../moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemoryPerSentence.cpp \
../moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerOnDisk.cpp \
../moses/TranslationModel/CYKPlusParser/DotChartInMemory.cpp \
../moses/TranslationModel/CYKPlusParser/DotChartOnDisk.cpp 

OBJS += \
./moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerCYKPlus.o \
./moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemory.o \
./moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemoryPerSentence.o \
./moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerOnDisk.o \
./moses/TranslationModel/CYKPlusParser/DotChartInMemory.o \
./moses/TranslationModel/CYKPlusParser/DotChartOnDisk.o 

CPP_DEPS += \
./moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerCYKPlus.d \
./moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemory.d \
./moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemoryPerSentence.d \
./moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerOnDisk.d \
./moses/TranslationModel/CYKPlusParser/DotChartInMemory.d \
./moses/TranslationModel/CYKPlusParser/DotChartOnDisk.d 


# Each subdirectory must supply rules for building sources it contributes
moses/TranslationModel/CYKPlusParser/%.o: ../moses/TranslationModel/CYKPlusParser/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


