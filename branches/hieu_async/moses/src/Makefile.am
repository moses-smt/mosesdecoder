INCLUDES = -I$(top_srcdir)/irstlm/src -I../../../srilm/include
METASOURCES = AUTO
lib_LIBRARIES = libmoses.a
libmoses_a_SOURCES = AlignmentElement.cpp AlignmentElement.h AlignmentPair.cpp \
			AlignmentPair.h AlignmentPhrase.cpp AlignmentPhrase.h ConfusionNet.cpp ConfusionNet.h \
			DecodeStep.cpp DecodeStep.h DecodeStepGeneration.cpp DecodeStepGeneration.h \
			DecodeStepTranslation.cpp DecodeStepTranslation.h Dictionary.cpp Dictionary.h \
			DummyScoreProducers.cpp DummyScoreProducers.h Factor.cpp Factor.h FactorCollection.cpp \
			FactorCollection.h FactorMask.cpp FactorMask.h File.cpp File.h FilePtr.h \
			GenerationDictionary.cpp GenerationDictionary.h Hypothesis.cpp Hypothesis.h HypothesisStack.cpp \
			HypothesisStack.h HypothesisStackCollection.cpp HypothesisStackCollection.h \
			InputFileStream.cpp InputFileStream.h InputType.cpp InputType.h LMList.cpp LMList.h \
			LanguageModel.cpp LanguageModel.h LanguageModelFactory.cpp LanguageModelFactory.h \
			LanguageModelIRST.cpp LanguageModelIRST.h LanguageModelInternal.cpp LanguageModelInternal.h \
			LanguageModelJoint.cpp LanguageModelJoint.h LanguageModelMultiFactor.cpp \
			LanguageModelMultiFactor.h LanguageModelSRI.cpp LanguageModelSRI.h LanguageModelSingleFactor.cpp \
			LanguageModelSingleFactor.h LanguageModelSkip.cpp LanguageModelSkip.h LexicalReordering.cpp \
			LexicalReordering.h Manager.cpp Manager.h NGramCollection.cpp NGramCollection.h \
			NGramNode.cpp NGramNode.h ObjectPool.h PDTAimp.h Parameter.cpp Parameter.h \
			PartialTranslOptColl.cpp PartialTranslOptColl.h Phrase.cpp Phrase.h PhraseDictionary.cpp \
			PhraseDictionary.h PhraseDictionaryMemory.cpp PhraseDictionaryMemory.h \
			PhraseDictionaryNode.cpp PhraseDictionaryNode.h PhraseDictionaryTree.cpp PhraseDictionaryTree.h \
			PhraseDictionaryTreeAdaptor.cpp PhraseDictionaryTreeAdaptor.h PhraseList.cpp PhraseList.h \
			PhraseReference.cpp PhraseReference.h PrefixPhraseCollection.cpp PrefixPhraseCollection.h \
			PrefixTree.h ScoreComponentCollection.cpp ScoreComponentCollection.h \
			ScoreIndexManager.cpp ScoreIndexManager.h ScoreProducer.cpp ScoreProducer.h Sentence.cpp \
			Sentence.h SentenceStats.cpp SentenceStats.h SpanScore.cpp SpanScore.h \
			SquareMatrix.cpp SquareMatrix.h StaticData.cpp StaticData.h TargetPhrase.cpp \
			TargetPhrase.h TargetPhraseCollection.cpp TargetPhraseCollection.h Timer.h \
			TranslationOption.cpp TranslationOption.h TranslationOptionCollection.cpp \
			TranslationOptionCollection.h TranslationOptionCollectionConfusionNet.cpp \
			TranslationOptionCollectionConfusionNet.h TranslationOptionCollectionText.cpp TranslationOptionCollectionText.h \
			TranslationOptionList.cpp TranslationOptionList.h TrellisPath.cpp TrellisPath.h \
			TrellisPathCollection.cpp TrellisPathCollection.h TrellisPathList.h TypeDef.h UniqueObject.h \
			UserMessage.cpp UserMessage.h Util.cpp Util.h Word.cpp Word.h WordsBitmap.cpp \
			WordsBitmap.h WordsRange.cpp WordsRange.h gzfilebuf.h hash.cpp hash.h md5.cpp md5.h \
			mempool.cpp mempool.h DecodeStepCollection.cpp DecodeStepCollection.h PDTAimp.cpp


AM_CXXFLAGS = -DLM_SRI -DLM_IRST -DLM_INTERNAL\
	-DTRACE_ENABLE	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
