lib_LIBRARIES = libmoses.a
libmoses_a_SOURCES = \
	Arc.cpp \
	ConfusionNet.cpp \
	CreateTargetPhraseCollection.cpp \
	Factor.cpp \
	FactorCollection.cpp \
	FactorTypeSet.cpp \
	GenerationDictionary.cpp \
	Hypothesis.cpp \
	HypothesisCollection.cpp \
	HypothesisCollectionIntermediate.cpp \
	Input.cpp \
	InputFileStream.cpp \
	LanguageModel.cpp \
	LatticeEdge.cpp \
	LatticePath.cpp \
	LexicalReordering.cpp \
	Manager.cpp \
	md5.cpp \
	Parameter.cpp \
	Phrase.cpp \
	PhraseDictionary.cpp \
	PhraseDictionaryTree.cpp \
	ScoreComponent.cpp \
	Sentence.cpp \
	StaticData.cpp \
	TargetPhrase.cpp \
	TranslationOption.cpp \
	TranslationOptionCollection.cpp \
	TranslationOptionCollectionText.cpp \
	TranslationOptionCollectionConfusionNet.cpp \
	UnknownWordHandler.cpp \
 	UserMessage.cpp \
 	Util.cpp \
 	WeightOptimization.cpp \
 	Word.cpp \
 	WordsBitmap.cpp \
	WordsRange.cpp

if INTERNAL_LM
libmoses_a_SOURCES += NGramCollection.cpp LanguageModel_Internal.cpp
endif

if SRI_LM
libmoses_a_SOURCES += LanguageModel_SRI.cpp
endif

#if IRST_LM
#libmoses_a_SOURCES += LanguageModel_IRST.cpp
#endif

