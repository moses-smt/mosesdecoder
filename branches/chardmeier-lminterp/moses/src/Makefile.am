lib_LIBRARIES = libmoses.a
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES 
libmoses_a_SOURCES = \
	AlignmentElement.cpp \
	AlignmentPhrase.cpp \
	AlignmentPair.cpp \
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
	LexicalReorderingTable.cpp \
	Manager.cpp \
	mempool.cpp \
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

libmoses_a_SOURCES += rule.pb.cc hypergraph.pb.cc

endif

if SRI_LM
libmoses_a_SOURCES += LanguageModelSRI.cpp
endif

if IRST_LM
libmoses_a_SOURCES += LanguageModelIRST.cpp
endif

if RAND_LM
libmoses_a_SOURCES += LanguageModelRandLM.cpp
endif

if INTERNAL_LM
libmoses_a_SOURCES += LanguageModelInternal.cpp \
		      NGramCollection.cpp \
		      NGramNode.cpp
endif
