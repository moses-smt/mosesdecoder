lib_LIBRARIES = libmoses.a
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES 
libmoses_a_SOURCES = \
AlignmentElement.cpp \
AlignmentPair.cpp \
AlignmentPhrase.cpp \
ConcatenatedPhrase.cpp \
ConfusionNet.cpp \
DecodeGraph.cpp \
DecodeStep.cpp \
DecodeStepGeneration.cpp \
DecodeStepTranslation.cpp \
Dictionary.cpp \
DummyScoreProducers.cpp \
FactorCollection.cpp \
Factor.cpp \
FactorMask.cpp \
File.cpp \
FloydWarshall.cpp \
GenerationDictionary.cpp \
hash.cpp \
Hypothesis.cpp \
HypothesisStack.cpp \
InputFileStream.cpp \
InputType.cpp \
LanguageModel.cpp \
LanguageModelFactory.cpp \
LanguageModelJoint.cpp \
LanguageModelMultiFactor.cpp \
LanguageModelSingleFactor.cpp \
LanguageModelSkip.cpp \
LexicalReordering.cpp \
LexicalReorderingTable.cpp \
LMList.cpp \
LVoc.cpp \
Manager.cpp \
md5.cpp \
mempool.cpp \
Parameter.cpp \
PartialTranslOptColl.cpp \
PCNTools.cpp \
Phrase.cpp \
PhraseDictionary.cpp \
PhraseDictionaryMemory.cpp \
PhraseDictionaryNode.cpp \
PhraseDictionaryTreeAdaptor.cpp \
PhraseDictionaryTree.cpp \
PhraseList.cpp \
PrefixPhraseCollection.cpp \
PrefixTreeMap.cpp \
ScoreComponentCollection.cpp \
ScoreIndexManager.cpp \
ScoreProducer.cpp \
Sentence.cpp \
SentenceStats.cpp \
SquareMatrix.cpp \
StaticData.cpp \
TargetPhraseCollection.cpp \
TargetPhrase.cpp \
TargetPhraseMatrix.cpp \
TranslationOptionCollectionConfusionNet.cpp \
TranslationOptionCollection.cpp \
TranslationOptionCollectionText.cpp \
TranslationOption.cpp \
TranslationOptionList.cpp \
TrellisPathCollection.cpp \
TrellisPath.cpp \
UserMessage.cpp \
Util.cpp \
Word.cpp \
WordLattice.cpp \
WordsBitmap.cpp \
WordsRange.cpp \
XmlOption.cpp 



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
