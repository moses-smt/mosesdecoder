################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../phrase-extract/extract-ghkm/Alignment.cpp \
../phrase-extract/extract-ghkm/AlignmentGraph.cpp \
../phrase-extract/extract-ghkm/ComposedRule.cpp \
../phrase-extract/extract-ghkm/ExtractGHKM.cpp \
../phrase-extract/extract-ghkm/Main.cpp \
../phrase-extract/extract-ghkm/Node.cpp \
../phrase-extract/extract-ghkm/ParseTree.cpp \
../phrase-extract/extract-ghkm/ScfgRule.cpp \
../phrase-extract/extract-ghkm/ScfgRuleWriter.cpp \
../phrase-extract/extract-ghkm/Span.cpp \
../phrase-extract/extract-ghkm/Subgraph.cpp \
../phrase-extract/extract-ghkm/XmlTreeParser.cpp 

OBJS += \
./phrase-extract/extract-ghkm/Alignment.o \
./phrase-extract/extract-ghkm/AlignmentGraph.o \
./phrase-extract/extract-ghkm/ComposedRule.o \
./phrase-extract/extract-ghkm/ExtractGHKM.o \
./phrase-extract/extract-ghkm/Main.o \
./phrase-extract/extract-ghkm/Node.o \
./phrase-extract/extract-ghkm/ParseTree.o \
./phrase-extract/extract-ghkm/ScfgRule.o \
./phrase-extract/extract-ghkm/ScfgRuleWriter.o \
./phrase-extract/extract-ghkm/Span.o \
./phrase-extract/extract-ghkm/Subgraph.o \
./phrase-extract/extract-ghkm/XmlTreeParser.o 

CPP_DEPS += \
./phrase-extract/extract-ghkm/Alignment.d \
./phrase-extract/extract-ghkm/AlignmentGraph.d \
./phrase-extract/extract-ghkm/ComposedRule.d \
./phrase-extract/extract-ghkm/ExtractGHKM.d \
./phrase-extract/extract-ghkm/Main.d \
./phrase-extract/extract-ghkm/Node.d \
./phrase-extract/extract-ghkm/ParseTree.d \
./phrase-extract/extract-ghkm/ScfgRule.d \
./phrase-extract/extract-ghkm/ScfgRuleWriter.d \
./phrase-extract/extract-ghkm/Span.d \
./phrase-extract/extract-ghkm/Subgraph.d \
./phrase-extract/extract-ghkm/XmlTreeParser.d 


# Each subdirectory must supply rules for building sources it contributes
phrase-extract/extract-ghkm/%.o: ../phrase-extract/extract-ghkm/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


