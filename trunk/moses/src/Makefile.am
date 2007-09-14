lib_LIBRARIES = libmoses.a
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES 
libmoses_a_SOURCES = \
	ConfusionNet.cpp \
	DecodeGraph.cpp \
	DecodeStep.cpp \
	DecodeStepGeneration.cpp \
	DecodeStepTranslation.cpp \
	Dictionary.cpp \
	DummyScoreProducers.cpp \
	Factor.cpp \
	FactorCollection.cpp \
	FactorTypeSet.cpp \
	FloydWarshall.cpp \
	GenerationDictionary.cpp \
	hash.cpp \
	Hypothesis.cpp \
	HypothesisStack.cpp \
	InputType.cpp \
	InputFileStream.cpp \
	LMList.cpp \
	LVoc.cpp \
	LanguageModel.cpp \
	LanguageModelFactory.cpp \
	LanguageModelMultiFactor.cpp \
	LanguageModelSingleFactor.cpp \
	LanguageModelSkip.cpp \
	TrellisPath.cpp \
	TrellisPathCollection.cpp \
	LexicalReordering.cpp \
	LexicalReorderingTable.cpp \
	Manager.cpp \
	md5.cpp \
	mempool.cpp \
	PCNTools.cpp \
	Parameter.cpp \
	PartialTranslOptColl.cpp \
	Phrase.cpp \
	PhraseDictionary.cpp \
	PhraseDictionaryMemory.cpp \
	PhraseDictionaryNode.cpp \
	PhraseDictionaryTree.cpp \
	PhraseDictionaryTreeAdaptor.cpp \
	PhraseReference.cpp \
	PrefixTreeMap.cpp \
	ScoreComponentCollection.cpp \
	ScoreIndexManager.cpp \
	ScoreProducer.cpp \
	Sentence.cpp \
	SentenceStats.cpp \
	StaticData.cpp \
	TargetPhrase.cpp \
	TargetPhraseCollection.cpp \
	TranslationOption.cpp \
	TranslationOptionCollection.cpp \
	TranslationOptionCollectionText.cpp \
	TranslationOptionCollectionConfusionNet.cpp \
	UserMessage.cpp \
	Util.cpp \
	Word.cpp \
	WordsBitmap.cpp \
	WordLattice.cpp \
	WordsRange.cpp


if SRI_LM
libmoses_a_SOURCES += LanguageModelSRI.cpp
endif

if IRST_LM
libmoses_a_SOURCES += LanguageModelIRST.cpp
endif

if INTERNAL_LM
libmoses_a_SOURCES += LanguageModelInternal.cpp \
		      NGramCollection.cpp \
		      NGramNode.cpp
endif
