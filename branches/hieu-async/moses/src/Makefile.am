INCLUDES = -I$(top_srcdir)/irstlm/src -I../../../srilm/include
METASOURCES = AUTO
lib_LIBRARIES = libmoses.a
libmoses_a_SOURCES = ConfusionNet.cpp ConfusionNet.h DecodeStep.cpp \
		DecodeStepGeneration.cpp DecodeStepGeneration.h DecodeStep.h DecodeStepTranslation.cpp \
		DecodeStepTranslation.h Dictionary.cpp Dictionary.h DummyScoreProducers.cpp \
		DummyScoreProducers.h FactorCollection.cpp FactorCollection.h Factor.cpp Factor.h \
		FactorMask.cpp FactorMask.h File.cpp File.h FilePtr.h GenerationDictionary.cpp \
		GenerationDictionary.h gzfilebuf.h hash.cpp hash.h HypothesisCollection.cpp \
		HypothesisCollection.h Hypothesis.cpp Hypothesis.h HypothesisStack.cpp HypothesisStack.h \
		InputFileStream.cpp InputFileStream.h InputType.cpp InputType.h LanguageModel.cpp \
		LanguageModelFactory.cpp LanguageModelFactory.h LanguageModel.h LanguageModelInternal.cpp \
		LanguageModelInternal.h LanguageModelIRST.cpp LanguageModelIRST.h LanguageModelJoint.cpp \
		LanguageModelJoint.h LanguageModelMultiFactor.cpp LanguageModelMultiFactor.h \
		LanguageModelSingleFactor.cpp LanguageModelSingleFactor.h LanguageModelSkip.cpp LanguageModelSkip.h \
		LanguageModelSRI.cpp LanguageModelSRI.h LatticePathCollection.h LatticePath.cpp \
		LatticePath.h LatticePathList.h LexicalReordering.cpp LexicalReordering.h LMList.cpp \
		LMList.h Manager.cpp Manager.h md5.cpp md5.h mempool.cpp mempool.h \
		NGramCollection.cpp NGramCollection.h NGramNode.cpp NGramNode.h ObjectPool.h Parameter.cpp \
		Parameter.h PartialTranslOptColl.cpp PartialTranslOptColl.h PDTAimp.h Phrase.cpp \
		PhraseDictionary.cpp PhraseDictionary.h PhraseDictionaryMemory.cpp PhraseDictionaryMemory.h \
		PhraseDictionaryNode.cpp PhraseDictionaryNode.h PhraseDictionaryTreeAdaptor.cpp \
		PhraseDictionaryTreeAdaptor.h PhraseDictionaryTree.cpp PhraseDictionaryTree.h Phrase.h \
		PhraseReference.cpp PhraseReference.h PrefixTree.h ScoreComponentCollection.cpp \
		ScoreComponentCollection.h ScoreIndexManager.cpp ScoreIndexManager.h ScoreProducer.cpp \
		ScoreProducer.h Sentence.cpp Sentence.h SentenceStats.cpp SentenceStats.h SpanScore.cpp \
		SpanScore.h SquareMatrix.cpp SquareMatrix.h StaticData.cpp StaticData.h \
		TargetPhraseCollection.cpp TargetPhraseCollection.h TargetPhrase.cpp TargetPhrase.h Timer.h \
		TranslationOptionCollectionConfusionNet.cpp TranslationOptionCollectionConfusionNet.h \
		TranslationOptionCollection.cpp TranslationOptionCollection.h TranslationOptionCollectionText.cpp \
		TranslationOptionCollectionText.h TranslationOption.cpp TranslationOption.h TypeDef.h UniqueObject.h \
		UserMessage.cpp UserMessage.h Util.cpp Util.h Word.cpp Word.h WordsBitmap.cpp \
		WordsBitmap.h WordsRange.cpp WordsRange.h AlignmentPair.cpp AlignmentPair.h \
		AlignmentPhrase.cpp AlignmentPhrase.h PhraseList.cpp PhraseList.h LatticePathCollection.cpp \
		PrefixPhraseCollection.cpp PrefixPhraseCollection.h AlignmentElement.cpp AlignmentElement.h


AM_CXXFLAGS = -DLM_SRI -DLM_IRST -DLM_INTERNAL\
	-DTRACE_ENABLE	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
