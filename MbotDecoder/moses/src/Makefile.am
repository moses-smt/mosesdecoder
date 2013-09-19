lib_LTLIBRARIES = libmoses.la
AM_CPPFLAGS = -w -g -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES $(BOOST_CPPFLAGS)

libmoses_ladir = ${includedir}

libmoses_la_HEADERS = \
        AlignmentInfo.h \
	AlignmentInfoMBOT.h \
        AlignmentInfoCollection.h \
	AlignmentInfoCollectionMBOT.h \
        BilingualDynSuffixArray.h \
        BitmapContainer.h \
        CellCollection.h \
	ChartCell.h \
ChartCellMBOT.h \
ChartCellLabelMBOT.h \
ChartCellLabelSetMBOT.h \
	ChartCellCollection.h \
	ChartHypothesis.h \
ChartHypothesisMBOT.h \
	ChartHypothesisCollection.h \
ChartHypothesisCollectionMBOT.h \
	ChartManager.h \
        ChartRuleLookupManager.h \
        ChartRuleLookupManagerMemory.h \
ChartRuleLookupManagerMemoryMBOT.h \
        ChartRuleLookupManagerOnDisk.h \
        ChartTranslationOption.h \
ChartTranslationOptionMBOT.h \
	ChartTranslationOptionCollection.h \
        ChartTranslationOptionList.h \
	ChartTrellisDetour.h \
ChartTreillisDetourMBOT.h \
	ChartTrellisDetourQueue.h \
ChartTreillisDetourQueueMBOT.h \
	ChartTrellisNode.h \
ChartTreillisNodeMBOT.h \
	ChartTrellisPath.h \
ChartTreillisPathMBOT.h \
	ChartTrellisPathList.h \
ChartTreillisPathListMBOT.h \
        ConfusionNet.h \
        DecodeFeature.h \
        DecodeGraph.h \
        DecodeStep.h \
        DecodeStepGeneration.h \
        DecodeStepTranslation.h \
        Dictionary.h \
        DotChart.h \
	ContextFactor.h \
DotChartMBOT.h \
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
        File.h \
        FilePtr.h \
        FloydWarshall.h \
	GapPenaltyProducer.h \
        GenerationDictionary.h \
        GlobalLexicalModel.h \
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
	MBOTWord.h \
        NonTerminal.h \
        ObjectPool.h \
        PCNTools.h \
        PDTAimp.h \
        Parameter.h \
        PartialTranslOptColl.h \
        Phrase.h \
        PhraseDictionary.h \
        PhraseDictionaryALSuffixArray.h \
        PhraseDictionaryDynSuffixArray.h \
        PhraseDictionaryMemory.h \
PhraseDictionaryMBOT.h \
        PhraseDictionarySCFG.h \
        PhraseDictionaryNode.h \
        PhraseDictionaryNodeSCFG.h \
PhraseDictionaryNodeMBOT.h \
        PhraseDictionaryOnDisk.h \
        PhraseDictionaryTree.h \
        PhraseDictionaryTreeAdaptor.h \
        PrefixTree.h \
        PrefixTreeMap.h \
ProcessedNonTerminals.h \
        ReorderingConstraint.h \
        ReorderingStack.h \
        RuleCube.h \
RuleCubeMBOT.h \
        RuleCubeItem.h \
RuleCubeItemMBOT.h \
        RuleCubeQueue.h \
RuleCubeQueueMBOT.h \
        RuleTableLoader.h \
        RuleTableLoaderCompact.h \
        RuleTableLoaderFactory.h \
        RuleTableLoaderHiero.h \
	RuleTableLoaderMBOT.h \
        RuleTableLoaderStandard.h \
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
TargetPhraseMBOT.h \
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
libmoses_la_HEADERS += LM/Rand.h
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
	AlignmentInfoMBOT.cpp \
        AlignmentInfoCollection.cpp \
	AlignmentInfoCollectionMBOT.cpp \
        BilingualDynSuffixArray.cpp \
        BitmapContainer.cpp \
	ChartCell.cpp \
ChartCellMBOT.cpp \
	ChartCellCollection.cpp \
	ChartHypothesis.cpp \
ChartHypothesisMBOT.cpp \
	ChartHypothesisCollection.cpp \
ChartHypothesisCollectionMBOT.cpp \
	ChartManager.cpp \
	ChartRuleLookupManager.cpp \
        ChartRuleLookupManagerMemory.cpp \
ChartRuleLookupManagerMemoryMBOT.cpp \
        ChartRuleLookupManagerOnDisk.cpp \
        ChartTranslationOption.cpp \
ChartTranslationOptionMBOT.cpp \
	ChartTranslationOptionCollection.cpp \
        ChartTranslationOptionList.cpp \
	ChartTrellisDetour.cpp \
ChartTreillisDetourMBOT.cpp \
	ChartTrellisDetourQueue.cpp \
ChartTreillisDetourQueueMBOT.cpp \
	ChartTrellisNode.cpp \
ChartTreillisNodeMBOT.cpp \
	ChartTrellisPath.cpp \
ChartTreillisPathMBOT.cpp \
        ConfusionNet.cpp \
	ContextFactor.cpp \
        DecodeFeature.cpp \
        DecodeGraph.cpp \
        DecodeStep.cpp \
        DecodeStepGeneration.cpp \
        DecodeStepTranslation.cpp \
        Dictionary.cpp \
        DotChart.cpp \
DotChartMBOT.cpp \
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
        FloydWarshall.cpp \
	GapPenaltyProducer.cpp \
        GenerationDictionary.cpp \
        GlobalLexicalModel.cpp \
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
        PCNTools.cpp \
        Parameter.cpp \
        PartialTranslOptColl.cpp \
        Phrase.cpp \
        PhraseDictionary.cpp \
		PhraseDictionaryALSuffixArray.cpp \
		PhraseDictionaryDynSuffixArray.cpp \
        PhraseDictionaryHiero.cpp \
        PhraseDictionaryMemory.cpp \
        PhraseDictionarySCFG.cpp \
PhraseDictionaryMBOT.cpp \
        PhraseDictionaryNode.cpp \
        PhraseDictionaryNodeSCFG.cpp \
PhraseDictionaryNodeMBOT.cpp \
        PhraseDictionaryOnDisk.cpp \
        PhraseDictionaryTree.cpp \
        PhraseDictionaryTreeAdaptor.cpp \
        PrefixTreeMap.cpp \
        ReorderingConstraint.cpp \
        ReorderingStack.cpp \
        RuleCube.cpp \
RuleCubeMBOT.cpp \
        RuleCubeItem.cpp \
RuleCubeItemMBOT.cpp \
        RuleCubeQueue.cpp \
RuleCubeQueueMBOT.cpp \
        RuleTableLoaderCompact.cpp \
        RuleTableLoaderFactory.cpp \
        RuleTableLoaderHiero.cpp \
	RuleTableLoaderMBOT.cpp \
        RuleTableLoaderStandard.cpp \
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
	TargetPhraseMBOT.cpp \
        TargetPhraseCollection.cpp \
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
libmoses_la_SOURCES += LM/Rand.cpp
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
