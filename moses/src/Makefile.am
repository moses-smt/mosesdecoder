lib_LTLIBRARIES = libmoses.la
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES $(BOOST_CPPFLAGS)

libmoses_ladir = ${includedir}

libmoses_la_HEADERS = \
        AlignmentInfo.h \
        AlignmentInfoCollection.h \
        BilingualDynSuffixArray.h \
        BitmapContainer.h \
	BleuScoreFeature.h \
        CellCollection.h \
	ChartCell.h \
	ChartCellCollection.h \
	ChartHypothesis.h \
	ChartHypothesisCollection.h \
	ChartManager.h \
        ChartRuleLookupManager.h \
        ChartRuleLookupManagerMemory.h \
        ChartRuleLookupManagerOnDisk.h \
        ChartTranslationOption.h \
	ChartTranslationOptionCollection.h \
        ChartTranslationOptionList.h \
	ChartTrellisDetour.h \
	ChartTrellisDetourQueue.h \
	ChartTrellisNode.h \
	ChartTrellisPath.h \
	ChartTrellisPathList.h \
        ConfusionNet.h \
        DecodeFeature.h \
        DecodeGraph.h \
        DecodeStep.h \
        DecodeStepGeneration.h \
        DecodeStepTranslation.h \
        Dictionary.h \
        DotChart.h \
        DotChartInMemory.h \
        DotChartOnDisk.h \
        DummyScoreProducers.h \
	DynSAInclude/file.h \
	DynSAInclude/vocab.h \
        DynSuffixArray.h \
        FFState.h \
        Factor.h \
        FactorCollection.h \
        FactorTypeSet.h \
        FeatureFunction.h \
        FeatureVector.h \
        File.h \
        FilePtr.h \
        FloydWarshall.h \
        GenerationDictionary.h \
        GlobalLexicalModel.h \
	GlobalLexicalModelUnlimited.h \
        gzfilebuf.h \
        hash.h \
        Hypothesis.h \
        HypothesisStack.h \
        HypothesisStackCubePruning.h \
        HypothesisStackNormal.h \
        InputFileStream.h \
        InputType.h \
        LMList.h \
        LVoc.h \
        LM/Base.h \
        LM/Joint.h \
        LM/Factory.h \
        LM/Implementation.h \
        LM/MultiFactor.h \
        LM/Remote.h \
        LM/SingleFactor.h \
        LM/Ken.h \
        LexicalReordering.h \
        LexicalReorderingState.h \
        LexicalReorderingTable.h \
        Manager.h \
        NonTerminal.h \
        ObjectPool.h \
        OnlineCommand.h \
        PCNTools.h \
        PDTAimp.h \
        Parameter.h \
        PartialTranslOptColl.h \
        Phrase.h \
        PhraseBoundaryFeature.h \
        PhraseDictionary.h \
        PhraseDictionaryALSuffixArray.h \
        PhraseDictionaryDynSuffixArray.h \
        PhraseDictionaryMemory.h \
        PhraseDictionarySCFG.h \
        PhraseDictionaryNode.h \
        PhraseDictionaryNodeSCFG.h \
        PhraseDictionaryOnDisk.h \
        PhraseDictionaryTree.h \
        PhraseDictionaryTreeAdaptor.h \
        PhraseLengthFeature.h \
        PhrasePairFeature.h \
        PrefixTree.h \
        PrefixTreeMap.h \
        ReorderingConstraint.h \
        ReorderingStack.h \
        RuleCube.h \
        RuleCubeItem.h \
        RuleCubeQueue.h \
        RuleTableLoader.h \
        RuleTableLoaderCompact.h \
        RuleTableLoaderFactory.h \
        RuleTableLoaderHiero.h \
        RuleTableLoaderStandard.h \
        ScoreComponentCollection.h \
        ScoreProducer.h \
        Search.h \
        SearchCubePruning.h \
        SearchNormal.h \
        Sentence.h \
        SentenceStats.h \
        SourceWordDeletionFeature.h \
        SparsePhraseDictionaryFeature.h \
        SquareMatrix.h \
        StaticData.h \
        TargetBigramFeature.h \
	TargetNgramFeature.h \
        TargetPhrase.h \
        TargetPhraseCollection.h \
        TargetWordInsertionFeature.h \
        ThreadPool.h \
        Timer.h \
        TranslationOption.h \
        TranslationOptionCollection.h \
        TranslationOptionCollectionConfusionNet.h \
        TranslationOptionCollectionText.h \
        TranslationOptionList.h \
        TranslationSystem.h \
        TreeInput.h \
        TrellisPath.h \
        TrellisPathCollection.h \
        TrellisPathList.h \
        TypeDef.h \
        UniqueObject.h \
        UserMessage.h \
        Util.h \
        Word.h \
        WordLattice.h \
        WordTranslationFeature.h \
        WordsBitmap.h \
        WordsRange.h \
        XmlOption.h

if PROTOBUF
libmoses_la_HEADERS += rule.pb.h hypergraph.pb.h
endif

if SRI_LM
libmoses_la_HEADERS += LM/SRI.h \
                      LM/ParallelBackoff.h 
endif

if IRST_LM
libmoses_la_HEADERS += LM/IRST.h
endif

if RAND_LM
libmoses_la_HEADERS += LM/RandLM.h
endif

if ORLM_LM
libmoses_la_HEADERS += LM/ORLM.h \
		       DynSAInclude/params.h \
		       DynSAInclude/hash.h \
		       DynSAInclude/quantizer.h \
		       DynSAInclude/RandLMFilter.h \
		       DynSAInclude/RandLMCache.h
endif

if SYN_LM
libmoses_la_HEADERS += SyntacticLanguageModel.h
endif

libmoses_la_SOURCES = \
        AlignmentInfo.cpp \
        AlignmentInfoCollection.cpp \
        BilingualDynSuffixArray.cpp \
        BitmapContainer.cpp \
	      BleuScoreFeature.cpp \
	      ChartCell.cpp \
	      ChartCellCollection.cpp \
	      ChartHypothesis.cpp \
	      ChartHypothesisCollection.cpp \
	      ChartManager.cpp \
				ChartRuleLookupManager.cpp \
        ChartRuleLookupManagerMemory.cpp \
        ChartRuleLookupManagerOnDisk.cpp \
        ChartTranslationOption.cpp \
	      ChartTranslationOptionCollection.cpp \
        ChartTranslationOptionList.cpp \
        ChartTrellisDetour.cpp \
        ChartTrellisDetourQueue.cpp \
        ChartTrellisNode.cpp \
        ChartTrellisPath.cpp \
        ConfusionNet.cpp \
        DecodeFeature.cpp \
        DecodeGraph.cpp \
        DecodeStep.cpp \
        DecodeStepGeneration.cpp \
        DecodeStepTranslation.cpp \
        Dictionary.cpp \
        DotChart.cpp \
        DotChartInMemory.cpp \
        DotChartOnDisk.cpp \
        DummyScoreProducers.cpp \
	DynSAInclude/file.cpp \
	DynSAInclude/vocab.cpp \
        DynSuffixArray.cpp \
        FFState.cpp \
        Factor.cpp \
        FactorCollection.cpp \
        FactorTypeSet.cpp \
        FeatureFunction.cpp \
        FeatureVector.cpp \
        FloydWarshall.cpp \
        GenerationDictionary.cpp \
        GlobalLexicalModel.cpp \
	GlobalLexicalModelUnlimited.cpp \
        hash.cpp \
        Hypothesis.cpp \
        HypothesisStack.cpp \
        HypothesisStackCubePruning.cpp \
        HypothesisStackNormal.cpp \
        InputFileStream.cpp \
        InputType.cpp \
        LMList.cpp \
        LVoc.cpp \
        LM/Base.cpp \
        LM/Factory.cpp \
        LM/Implementation.cpp \
        LM/Joint.cpp \
				LM/Ken.cpp \
        LM/MultiFactor.cpp \
        LM/Remote.cpp \
        LM/SingleFactor.cpp \
        LexicalReordering.cpp \
        LexicalReorderingState.cpp \
        LexicalReorderingTable.cpp \
        Manager.cpp \
        OnlineCommand.cpp \
        PCNTools.cpp \
        Parameter.cpp \
        PartialTranslOptColl.cpp \
        Phrase.cpp \
        PhraseBoundaryFeature.cpp \
        PhraseDictionary.cpp \
		PhraseDictionaryALSuffixArray.cpp \
		PhraseDictionaryDynSuffixArray.cpp \
        PhraseDictionaryHiero.cpp \
        PhraseDictionaryMemory.cpp \
        PhraseDictionarySCFG.cpp \
        PhraseDictionaryNode.cpp \
        PhraseDictionaryNodeSCFG.cpp \
        PhraseDictionaryOnDisk.cpp \
        PhraseDictionaryTree.cpp \
        PhraseDictionaryTreeAdaptor.cpp \
        PhraseLengthFeature.cpp \
        PhrasePairFeature.cpp \
        PrefixTreeMap.cpp \
        ReorderingConstraint.cpp \
        ReorderingStack.cpp \
        RuleCube.cpp \
        RuleCubeItem.cpp \
        RuleCubeQueue.cpp \
        RuleTableLoaderCompact.cpp \
        RuleTableLoaderFactory.cpp \
        RuleTableLoaderHiero.cpp \
        RuleTableLoaderStandard.cpp \
        ScoreComponentCollection.cpp \
        ScoreProducer.cpp \
        Search.cpp \
        SearchCubePruning.cpp \
        SearchNormal.cpp \
        Sentence.cpp \
        SentenceStats.cpp \
        SourceWordDeletionFeature.cpp \
        SparsePhraseDictionaryFeature.cpp \
        SquareMatrix.cpp \
        StaticData.cpp \
        TargetBigramFeature.cpp \
	TargetNgramFeature.cpp \
        TargetPhrase.cpp \
        TargetPhraseCollection.cpp \
        TargetWordInsertionFeature.cpp \
        ThreadPool.cpp \
        Timer.cpp \
        TranslationOption.cpp \
        TranslationOptionCollection.cpp \
        TranslationOptionCollectionConfusionNet.cpp \
        TranslationOptionCollectionText.cpp \
        TranslationOptionList.cpp \
        TranslationSystem.cpp \
        TreeInput.cpp \
        TrellisPath.cpp \
        TrellisPathCollection.cpp \
        UserMessage.cpp \
        Util.cpp \
        Word.cpp \
        WordLattice.cpp \
        WordTranslationFeature.cpp \
        WordsBitmap.cpp \
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
libmoses_la_SOURCES += LM/SRI.cpp \
                      LM/ParallelBackoff.cpp 

endif

if IRST_LM
libmoses_la_SOURCES += LM/IRST.cpp
endif

if RAND_LM
libmoses_la_SOURCES += LM/RandLM.cpp
endif

if ORLM_LM
libmoses_la_SOURCES += LM/ORLM.cpp \
		       DynSAInclude/onlineRLM.h \
		       DynSAInclude/perfecthash.h \
		       DynSAInclude/params.cpp 
endif

if SYN_LM
libmoses_la_SOURCES += SyntacticLanguageModel.cpp
endif

libmoses_la_LIBADD = $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS)
