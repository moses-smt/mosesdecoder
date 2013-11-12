UNAME_S := $(shell uname -s)

BOOST_DIR = $(PWD)/../3rdparty/boost
BOOST_LIB = $(BOOST_DIR)/lib64
SRI_DIR = $(PWD)/../3rdparty/srilm
SRI_MACHINE_TYPE=$(shell $(SRI_DIR)/sbin/machine-type)
SRI_LIB = $(SRI_DIR)/lib/$(SRI_MACHINE_TYPE)

CXX = g++
CPPFLAGS = -Wall -O3 -I. -I$(SRI_DIR)/include -I$(BOOST_DIR)/include -DSCORE_BREAKDOWN
LIBS = -L$(BOOST_LIB) -L$(SRI_LIB) -lz -loolm -ldstruct -lflm -llattice -lmisc -lboost_system-mt -lpthread #-liconv

OBJECTS = Global.o		Phrase.o		Timer.o		Word.o \
	InputFileStream.o	Scores.o		TypeDef.o		WordsBitmap.o \
	InputPath.o		Sentence.o		Util.o		WordsRange.o \
	Main.o		TargetPhrase.o	MyVocab.o \
	TargetPhrases.o	Weights.o \
	FF/DistortionScoreProducer.o	FF/PhrasePenalty.o		FF/WordPenaltyProducer.o \
	FF/StatefulFeatureFunction.o \
	FF/FeatureFunction.o		FF/StatelessFeatureFunction.o \
	FF/TranslationModel/PhraseTable.o		FF/TranslationModel/UnknownWordPenalty.o \
	FF/TranslationModel/PhraseTableMemory.o \
	FF/TranslationModel/Memory/Node.o \
	FF/LM/LM.o FF/LM/InternalLM.o FF/LM/SRILM.o \
	Search/Hypothesis.o Search/Manager.o Search/Stack.o Search/Stacks.o

basic-decoder: $(OBJECTS)
	$(CXX) $(CPPFLAGS) -o basic-decoder $(OBJECTS) $(LIBS)
        #rm -f *.o */*.o */*/*.o */*/*/*.o

clean:
	rm -f basic-decoder *.o */*.o */*/*.o */*/*/*.o
