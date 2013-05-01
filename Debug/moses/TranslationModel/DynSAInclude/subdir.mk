################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../moses/TranslationModel/DynSAInclude/FileHandler.cpp \
../moses/TranslationModel/DynSAInclude/params.cpp \
../moses/TranslationModel/DynSAInclude/vocab.cpp 

OBJS += \
./moses/TranslationModel/DynSAInclude/FileHandler.o \
./moses/TranslationModel/DynSAInclude/params.o \
./moses/TranslationModel/DynSAInclude/vocab.o 

CPP_DEPS += \
./moses/TranslationModel/DynSAInclude/FileHandler.d \
./moses/TranslationModel/DynSAInclude/params.d \
./moses/TranslationModel/DynSAInclude/vocab.d 


# Each subdirectory must supply rules for building sources it contributes
moses/TranslationModel/DynSAInclude/%.o: ../moses/TranslationModel/DynSAInclude/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


