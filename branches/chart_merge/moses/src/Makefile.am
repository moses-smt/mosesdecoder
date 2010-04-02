lib_LTLIBRARIES = libmoses.la
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES $(BOOST_CPPFLAGS)

libmoses_ladir = ${includedir}

libmoses_la_HEADERS = \
	TypeDef.h \
	PrefixTree.h \
	File.h \
	FilePtr.h \
	ObjectPool.h \
	BitmapContainer.h \
	ConfusionNet.h \
	DecodeGraph.h \
	DecodeStep.h \
	DecodeStepGeneration.h \
	DecodeStepTranslation.h \
	Dictionary.h \
	DummyScoreProducers.h \
	Factor.h \
	FactorCollection.h \
	FactorTypeSet.h \
	FeatureFunction.h \
	FFState.h \
	FloydWarshall.h \
	GenerationDictionary.h \
	GlobalLexicalModel.h \
	hash.h \
	Hypothesis.h \
	HypothesisStack.h \
	HypothesisStackCubePruning.h \
	HypothesisStackNormal.h \
	InputType.h \
	InputFileStream.h \
	LMList.h \
	LVoc.h \
	LanguageModel.h \
	LanguageModelFactory.h \
	LanguageModelInternal.h \
	LanguageModelMultiFactor.h \
	LanguageModelRemote.h \
	LanguageModelSingleFactor.h \
	LanguageModelSkip.h \
	TrellisPath.h \
	TrellisPathList.h \
	TrellisPathCollection.h \
	LexicalReordering.h \
	LexicalReorderingState.h \
	LexicalReorderingTable.h \
	Manager.h \
	NGramCollection.h \
	NGramNode.h \
	PCNTools.h \
	Parameter.h \
	PartialTranslOptColl.h \
	Phrase.h \
	PhraseDictionary.h \
	PhraseDictionaryMemory.h \
	PhraseDictionaryNode.h \
	PhraseDictionaryTree.h \
	PhraseDictionaryTreeAdaptor.h \
	PrefixTreeMap.h \
	ReorderingConstraint.h \
	ReorderingStack.h \
	ScoreComponentCollection.h \
	ScoreIndexManager.h \
	ScoreProducer.h \
	Search.h \
	SearchCubePruning.h \
	SearchNormal.h \
	Sentence.h \
	SentenceStats.h \
	SquareMatrix.h \
	StaticData.h \
	TargetPhrase.h \
	TargetPhraseCollection.h \
	Timer.h \
	TranslationOption.h \
	TranslationOptionCollection.h \
	TranslationOptionCollectionText.h \
	TranslationOptionCollectionConfusionNet.h \
	TranslationOptionList.h \
	UserMessage.h \
	Util.h \
	Word.h \
	WordsBitmap.h \
	WordLattice.h \
	WordsRange.h \
	XmlOption.h 

if PROTOBUF
libmoses_la_HEADERS += rule.pb.h hypergraph.pb.h
endif

if SRI_LM
libmoses_la_HEADERS += LanguageModelSRI.h
endif

if IRST_LM
libmoses_la_HEADERS += LanguageModelIRST.h
endif

if RAND_LM
libmoses_la_HEADERS += LanguageModelRandLM.h
endif

if INTERNAL_LM
libmoses_la_HEADERS += LanguageModelInternal.h \
		      NGramCollection.h \
		      NGramNode.h
endif

libmoses_la_SOURCES = \
	BitmapContainer.cpp \
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
	FeatureFunction.cpp \
	FFState.cpp \
	FloydWarshall.cpp \
	GenerationDictionary.cpp \
	GlobalLexicalModel.cpp \
	hash.cpp \
	Hypothesis.cpp \
	HypothesisStack.cpp \
	HypothesisStackCubePruning.cpp \
	HypothesisStackNormal.cpp \
	InputType.cpp \
	InputFileStream.cpp \
	LMList.cpp \
	LVoc.cpp \
	LanguageModel.cpp \
	LanguageModelFactory.cpp \
	LanguageModelInternal.cpp \
	LanguageModelMultiFactor.cpp \
	LanguageModelRemote.cpp \
	LanguageModelSingleFactor.cpp \
	LanguageModelSkip.cpp \
	TrellisPath.cpp \
	TrellisPathCollection.cpp \
	LexicalReordering.cpp \
	LexicalReorderingState.cpp \
	LexicalReorderingTable.cpp \
	Manager.cpp \
	NGramCollection.cpp \
	NGramNode.cpp \
	PCNTools.cpp \
	Parameter.cpp \
	PartialTranslOptColl.cpp \
	Phrase.cpp \
	PhraseDictionary.cpp \
	PhraseDictionaryMemory.cpp \
	PhraseDictionaryNode.cpp \
	PhraseDictionaryTree.cpp \
	PhraseDictionaryTreeAdaptor.cpp \
	PrefixTreeMap.cpp \
	ReorderingConstraint.cpp \
	ReorderingStack.cpp \
	ScoreComponentCollection.cpp \
	ScoreIndexManager.cpp \
	ScoreProducer.cpp \
	Search.cpp \
	SearchCubePruning.cpp \
	SearchNormal.cpp \
	Sentence.cpp \
	SentenceStats.cpp \
	SquareMatrix.cpp \
	StaticData.cpp \
	TargetPhrase.cpp \
	TargetPhraseCollection.cpp \
	Timer.cpp \
	TranslationOption.cpp \
	TranslationOptionCollection.cpp \
	TranslationOptionCollectionText.cpp \
	TranslationOptionCollectionConfusionNet.cpp \
	TranslationOptionList.cpp \
	UserMessage.cpp \
	Util.cpp \
	Word.cpp \
	WordsBitmap.cpp \
	WordLattice.cpp \
	WordsRange.cpp \
	XmlOption.cpp 
	
if PROTOBUF
BUILT_SOURCES = \
  rule.pb.h \
  rule.pb.cc \
  hypergraph.pb.h \
  hypergraph.pb.cc

CLEANFILES = $(BUILT_SOURCES)
SUFFIXES = .proto

rule.pb.cc: rule.proto
	@PROTOC@ --cpp_out=. $<
rule.pb.h: rule.proto
	@PROTOC@ --cpp_out=. $<

hypergraph.pb.cc: hypergraph.proto
	@PROTOC@ --cpp_out=. $<			
hypergraph.pb.h: hypergraph.proto
	@PROTOC@ --cpp_out=. $<			

libmoses_la_SOURCES += rule.pb.cc hypergraph.pb.cc

endif

if SRI_LM
libmoses_la_SOURCES += LanguageModelSRI.cpp
endif

if IRST_LM
libmoses_la_SOURCES += LanguageModelIRST.cpp
endif

if RAND_LM
libmoses_la_SOURCES += LanguageModelRandLM.cpp
endif

if INTERNAL_LM
libmoses_la_SOURCES += LanguageModelInternal.cpp \
		      NGramCollection.cpp \
		      NGramNode.cpp
endif

libmoses_la_LIBADD = $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)
