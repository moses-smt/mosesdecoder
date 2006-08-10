lib_LIBRARIES = libmoses.a
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES 
libmoses_a_SOURCES = \
	ConfusionNet.cpp \
	Dictionary.cpp \
	DistortionOrientation.cpp \
	DummyScoreProducers.cpp \
	Factor.cpp \
	FactorArrayWrapper.cpp \
	FactorCollection.cpp \
	FactorTypeSet.cpp \
	GenerationDictionary.cpp \
	hash.cpp \
	Hypothesis.cpp \
	HypothesisCollection.cpp \
	Input.cpp \
	InputFileStream.cpp \
	InputOutput.cpp \
	LMList.cpp \
	LanguageModel.cpp \
	LanguageModelFactory.cpp \
	LanguageModelMultiFactor.cpp \
	LanguageModelSingleFactor.cpp \
	LanguageModel_Chunking.cpp \
	LatticePath.cpp \
	LexicalReordering.cpp \
	Manager.cpp \
	md5.cpp \
	mempool.cpp \
	Parameter.cpp \
	PartialTranslOptColl.cpp \
	Phrase.cpp \
	PhraseDictionary.cpp \
	PhraseDictionaryBase.cpp \
	PhraseDictionaryNode.cpp \
	PhraseDictionaryTree.cpp \
	PhraseDictionaryTreeAdaptor.cpp \
	ScoreComponentCollection.cpp \
	ScoreIndexManager.cpp \
	ScoreProducer.cpp \
	Sentence.cpp \
	StaticData.cpp \
	TargetPhrase.cpp \
	TranslationOption.cpp \
	TranslationOptionCollection.cpp \
	TranslationOptionCollectionText.cpp \
	TranslationOptionCollectionConfusionNet.cpp \
	UserMessage.cpp \
	Util.cpp \
	Word.cpp \
	WordsBitmap.cpp \
	WordsRange.cpp


if INTERNAL_LM
libmoses_a_SOURCES += NGramCollection.cpp LanguageModel_Internal.cpp
endif

if SRI_LM
libmoses_a_SOURCES += LanguageModel_SRI.cpp
endif

if IRST_LM
libmoses_a_SOURCES += LanguageModel_IRST.cpp
endif

