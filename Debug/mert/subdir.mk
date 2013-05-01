################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../mert/BleuScorer.cpp \
../mert/BleuScorerTest.cpp \
../mert/CderScorer.cpp \
../mert/Data.cpp \
../mert/DataTest.cpp \
../mert/FeatureArray.cpp \
../mert/FeatureData.cpp \
../mert/FeatureDataIterator.cpp \
../mert/FeatureDataTest.cpp \
../mert/FeatureStats.cpp \
../mert/FileStream.cpp \
../mert/GzFileBuf.cpp \
../mert/HypPackEnumerator.cpp \
../mert/InterpolatedScorer.cpp \
../mert/MiraFeatureVector.cpp \
../mert/MiraWeightVector.cpp \
../mert/NgramTest.cpp \
../mert/Optimizer.cpp \
../mert/OptimizerFactory.cpp \
../mert/OptimizerFactoryTest.cpp \
../mert/PerScorer.cpp \
../mert/Permutation.cpp \
../mert/PermutationScorer.cpp \
../mert/Point.cpp \
../mert/PointTest.cpp \
../mert/PreProcessFilter.cpp \
../mert/ReferenceTest.cpp \
../mert/ScoreArray.cpp \
../mert/ScoreData.cpp \
../mert/ScoreDataIterator.cpp \
../mert/ScoreStats.cpp \
../mert/Scorer.cpp \
../mert/ScorerFactory.cpp \
../mert/SemposOverlapping.cpp \
../mert/SemposScorer.cpp \
../mert/SentenceLevelScorer.cpp \
../mert/SingletonTest.cpp \
../mert/StatisticsBasedScorer.cpp \
../mert/TerScorer.cpp \
../mert/Timer.cpp \
../mert/TimerTest.cpp \
../mert/Util.cpp \
../mert/UtilTest.cpp \
../mert/Vocabulary.cpp \
../mert/VocabularyTest.cpp \
../mert/evaluator.cpp \
../mert/extractor.cpp \
../mert/kbmira.cpp \
../mert/mert.cpp \
../mert/pro.cpp \
../mert/sentence-bleu.cpp 

OBJS += \
./mert/BleuScorer.o \
./mert/BleuScorerTest.o \
./mert/CderScorer.o \
./mert/Data.o \
./mert/DataTest.o \
./mert/FeatureArray.o \
./mert/FeatureData.o \
./mert/FeatureDataIterator.o \
./mert/FeatureDataTest.o \
./mert/FeatureStats.o \
./mert/FileStream.o \
./mert/GzFileBuf.o \
./mert/HypPackEnumerator.o \
./mert/InterpolatedScorer.o \
./mert/MiraFeatureVector.o \
./mert/MiraWeightVector.o \
./mert/NgramTest.o \
./mert/Optimizer.o \
./mert/OptimizerFactory.o \
./mert/OptimizerFactoryTest.o \
./mert/PerScorer.o \
./mert/Permutation.o \
./mert/PermutationScorer.o \
./mert/Point.o \
./mert/PointTest.o \
./mert/PreProcessFilter.o \
./mert/ReferenceTest.o \
./mert/ScoreArray.o \
./mert/ScoreData.o \
./mert/ScoreDataIterator.o \
./mert/ScoreStats.o \
./mert/Scorer.o \
./mert/ScorerFactory.o \
./mert/SemposOverlapping.o \
./mert/SemposScorer.o \
./mert/SentenceLevelScorer.o \
./mert/SingletonTest.o \
./mert/StatisticsBasedScorer.o \
./mert/TerScorer.o \
./mert/Timer.o \
./mert/TimerTest.o \
./mert/Util.o \
./mert/UtilTest.o \
./mert/Vocabulary.o \
./mert/VocabularyTest.o \
./mert/evaluator.o \
./mert/extractor.o \
./mert/kbmira.o \
./mert/mert.o \
./mert/pro.o \
./mert/sentence-bleu.o 

CPP_DEPS += \
./mert/BleuScorer.d \
./mert/BleuScorerTest.d \
./mert/CderScorer.d \
./mert/Data.d \
./mert/DataTest.d \
./mert/FeatureArray.d \
./mert/FeatureData.d \
./mert/FeatureDataIterator.d \
./mert/FeatureDataTest.d \
./mert/FeatureStats.d \
./mert/FileStream.d \
./mert/GzFileBuf.d \
./mert/HypPackEnumerator.d \
./mert/InterpolatedScorer.d \
./mert/MiraFeatureVector.d \
./mert/MiraWeightVector.d \
./mert/NgramTest.d \
./mert/Optimizer.d \
./mert/OptimizerFactory.d \
./mert/OptimizerFactoryTest.d \
./mert/PerScorer.d \
./mert/Permutation.d \
./mert/PermutationScorer.d \
./mert/Point.d \
./mert/PointTest.d \
./mert/PreProcessFilter.d \
./mert/ReferenceTest.d \
./mert/ScoreArray.d \
./mert/ScoreData.d \
./mert/ScoreDataIterator.d \
./mert/ScoreStats.d \
./mert/Scorer.d \
./mert/ScorerFactory.d \
./mert/SemposOverlapping.d \
./mert/SemposScorer.d \
./mert/SentenceLevelScorer.d \
./mert/SingletonTest.d \
./mert/StatisticsBasedScorer.d \
./mert/TerScorer.d \
./mert/Timer.d \
./mert/TimerTest.d \
./mert/Util.d \
./mert/UtilTest.d \
./mert/Vocabulary.d \
./mert/VocabularyTest.d \
./mert/evaluator.d \
./mert/extractor.d \
./mert/kbmira.d \
./mert/mert.d \
./mert/pro.d \
./mert/sentence-bleu.d 


# Each subdirectory must supply rules for building sources it contributes
mert/%.o: ../mert/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


