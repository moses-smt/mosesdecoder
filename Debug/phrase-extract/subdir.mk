################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../phrase-extract/AlignmentPhrase.cpp \
../phrase-extract/ExtractedRule.cpp \
../phrase-extract/HoleCollection.cpp \
../phrase-extract/InputFileStream.cpp \
../phrase-extract/OutputFileStream.cpp \
../phrase-extract/PhraseAlignment.cpp \
../phrase-extract/ScoreFeature.cpp \
../phrase-extract/ScoreFeatureTest.cpp \
../phrase-extract/SentenceAlignment.cpp \
../phrase-extract/SentenceAlignmentWithSyntax.cpp \
../phrase-extract/SyntaxTree.cpp \
../phrase-extract/XmlTree.cpp \
../phrase-extract/consolidate-direct-main.cpp \
../phrase-extract/consolidate-main.cpp \
../phrase-extract/consolidate-reverse-main.cpp \
../phrase-extract/domain.cpp \
../phrase-extract/extract-lex-main.cpp \
../phrase-extract/extract-main.cpp \
../phrase-extract/extract-rules-main.cpp \
../phrase-extract/relax-parse-main.cpp \
../phrase-extract/score-main.cpp \
../phrase-extract/statistics-main.cpp \
../phrase-extract/tables-core.cpp 

OBJS += \
./phrase-extract/AlignmentPhrase.o \
./phrase-extract/ExtractedRule.o \
./phrase-extract/HoleCollection.o \
./phrase-extract/InputFileStream.o \
./phrase-extract/OutputFileStream.o \
./phrase-extract/PhraseAlignment.o \
./phrase-extract/ScoreFeature.o \
./phrase-extract/ScoreFeatureTest.o \
./phrase-extract/SentenceAlignment.o \
./phrase-extract/SentenceAlignmentWithSyntax.o \
./phrase-extract/SyntaxTree.o \
./phrase-extract/XmlTree.o \
./phrase-extract/consolidate-direct-main.o \
./phrase-extract/consolidate-main.o \
./phrase-extract/consolidate-reverse-main.o \
./phrase-extract/domain.o \
./phrase-extract/extract-lex-main.o \
./phrase-extract/extract-main.o \
./phrase-extract/extract-rules-main.o \
./phrase-extract/relax-parse-main.o \
./phrase-extract/score-main.o \
./phrase-extract/statistics-main.o \
./phrase-extract/tables-core.o 

CPP_DEPS += \
./phrase-extract/AlignmentPhrase.d \
./phrase-extract/ExtractedRule.d \
./phrase-extract/HoleCollection.d \
./phrase-extract/InputFileStream.d \
./phrase-extract/OutputFileStream.d \
./phrase-extract/PhraseAlignment.d \
./phrase-extract/ScoreFeature.d \
./phrase-extract/ScoreFeatureTest.d \
./phrase-extract/SentenceAlignment.d \
./phrase-extract/SentenceAlignmentWithSyntax.d \
./phrase-extract/SyntaxTree.d \
./phrase-extract/XmlTree.d \
./phrase-extract/consolidate-direct-main.d \
./phrase-extract/consolidate-main.d \
./phrase-extract/consolidate-reverse-main.d \
./phrase-extract/domain.d \
./phrase-extract/extract-lex-main.d \
./phrase-extract/extract-main.d \
./phrase-extract/extract-rules-main.d \
./phrase-extract/relax-parse-main.d \
./phrase-extract/score-main.d \
./phrase-extract/statistics-main.d \
./phrase-extract/tables-core.d 


# Each subdirectory must supply rules for building sources it contributes
phrase-extract/%.o: ../phrase-extract/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


