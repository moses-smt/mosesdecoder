INCLUDES = -I$(top_srcdir)/irstlm/src -I$(top_srcdir)/srilm/include
METASOURCES = AUTO
lib_LIBRARIES = libmoses.a
libmoses_a_SOURCES = AlignmentElement.cpp \
AlignmentPair.cpp \
AlignmentPhrase.cpp \
ConfusionNet.cpp \
DecodeStepCollection.cpp \
DecodeStep.cpp \
DecodeStepGeneration.cpp \
DecodeStepTranslation.cpp \
Dictionary.cpp \
DummyScoreProducers.cpp \
FactorCollection.cpp \
Factor.cpp \
FactorMask.cpp \
File.cpp \
GenerationDictionary.cpp \
hash.cpp \
Hypothesis.cpp \
HypothesisStackCollection.cpp \
HypothesisStack.cpp \
InputFileStream.cpp \
InputType.cpp \
LanguageModel.cpp \
LanguageModelFactory.cpp \
LanguageModelInternal.cpp \
LanguageModelIRST.cpp \
LanguageModelJoint.cpp \
LanguageModelMultiFactor.cpp \
LanguageModelSingleFactor.cpp \
LanguageModelSkip.cpp \
LanguageModelSRI.cpp \
LexicalReordering.cpp \
LMList.cpp \
Manager.cpp \
md5.cpp \
mempool.cpp \
NGramCollection.cpp \
NGramNode.cpp \
Parameter.cpp \
PartialTranslOptColl.cpp \
PDTAimp.cpp \
Phrase.cpp \
PhraseDictionary.cpp \
PhraseDictionaryMemory.cpp \
PhraseDictionaryNode.cpp \
PhraseDictionaryTreeAdaptor.cpp \
PhraseDictionaryTree.cpp \
PhraseList.cpp \
PhraseReference.cpp \
PrefixPhraseCollection.cpp \
ScoreComponentCollection.cpp \
ScoreIndexManager.cpp \
ScoreProducer.cpp \
Sentence.cpp \
SentenceStats.cpp \
SpanScore.cpp \
SquareMatrix.cpp \
StaticData.cpp \
TargetPhraseCollection.cpp \
TargetPhrase.cpp \
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
WordsBitmap.cpp \
WordsRange.cpp 


AM_CXXFLAGS = -DLM_SRI -DLM_IRST -DLM_INTERNAL\
	-DTRACE_ENABLE	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
