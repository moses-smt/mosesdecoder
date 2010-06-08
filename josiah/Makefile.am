bin_PROGRAMS = josiah corpustrainer samplerank fv
josiah_SOURCES = Derivation.cpp \
	AnnealingSchedule.cpp \
	Decoder.cpp \
    Dependency.cpp \
    DiscriminativeLMFeature.cpp \
	InputSource.cpp \
	TrainingSource.cpp \
	Josiah.cpp \
	Optimizer.cpp \
	GainFunction.cpp \
        Gibbler.cpp \
        GibblerExpectedLossTraining.cpp \
        GibblerMaxDerivDecoder.cpp \
        GibblerMaxTransDecoder.cpp \
        GibbsOperator.cpp \
    MpiDebug.cpp \
	Model1.cpp \
    ParenthesisFeature.cpp \
    PosProjectionFeature.cpp \
    Pos.cpp \
    SourceToTargetRatio.cpp \
	TranslationDelta.cpp \
	SentenceBleu.cpp \
    FeatureFunction.cpp \
    FeatureVector.cpp \
    SearchRandom.cpp \
    SampleAcceptor.cpp \
    OnlineLearner.cpp \
    Sampler.cpp \
    SampleCollector.cpp \
    Utils.cpp \
    SufficientStats.cpp \
    GibbsOperatorIterator.cpp \
    GibblerAnnealedExpectedLossTrainer.cpp \
    WeightManager.cpp \
    WordPenaltyFeature.cpp \
    PhraseFeature.cpp \
    LanguageModelFeature.cpp \
    DistortionPenaltyFeature.cpp


#m1_SOURCES = Model1.cpp FeatureFunction.cpp m1.cpp

corpustrainer_SOURCES = Derivation.cpp \
        AnnealingSchedule.cpp \
        Decoder.cpp \
    Dependency.cpp \
    DiscriminativeLMFeature.cpp \
        InputSource.cpp \
        TrainingSource.cpp \
        Optimizer.cpp \
        GainFunction.cpp \
        Gibbler.cpp \
        GibblerExpectedLossTraining.cpp \
        GibblerMaxDerivDecoder.cpp \
        GibblerMaxTransDecoder.cpp \
        GibbsOperator.cpp \
    MpiDebug.cpp \
        Model1.cpp \
    Pos.cpp \
    SourceToTargetRatio.cpp \
        TranslationDelta.cpp \
        SentenceBleu.cpp \
    FeatureVector.cpp \
    FeatureFunction.cpp \
    SearchRandom.cpp \
    ParenthesisFeature.cpp \
    PosProjectionFeature.cpp \
    CorpusSampler.cpp \
    CorpusSamplerAnnealed.cpp \
    SampleAcceptor.cpp \
     Sampler.cpp \
    SampleCollector.cpp \
    Utils.cpp \
    SufficientStats.cpp \
    OnlineLearner.cpp \
    GibbsOperatorIterator.cpp \
    CorpusTrainer.cpp  \
    WeightManager.cpp \
    WordPenaltyFeature.cpp \
    PhraseFeature.cpp \
    LanguageModelFeature.cpp \
    DistortionPenaltyFeature.cpp
   

samplerank_SOURCES = Derivation.cpp \
        AnnealingSchedule.cpp \
        Decoder.cpp \
    Dependency.cpp \
    DiscriminativeLMFeature.cpp \
        InputSource.cpp \
        GainFunction.cpp \
        Gibbler.cpp \
        GibblerMaxDerivDecoder.cpp \
        GibblerMaxTransDecoder.cpp \
        GibbsOperator.cpp \
    MpiDebug.cpp \
        Model1.cpp \
    Pos.cpp \
    ParenthesisFeature.cpp \
    PosProjectionFeature.cpp \
    SourceToTargetRatio.cpp \
        TranslationDelta.cpp \
        SentenceBleu.cpp \
    FeatureFunction.cpp \
    FeatureVector.cpp \
    SearchRandom.cpp \
    SampleRank.cpp \
    OnlineLearner.cpp \
      Sampler.cpp \
    SampleCollector.cpp \
    Utils.cpp \
    SufficientStats.cpp \
   SampleAcceptor.cpp \
    GibbsOperatorIterator.cpp \
    OnlineLearnerTrainer.cpp  \
    WeightManager.cpp \
    WordPenaltyFeature.cpp \
    PhraseFeature.cpp \
    LanguageModelFeature.cpp \
    DistortionPenaltyFeature.cpp
   
fv_SOURCES = FeatureVector.cpp FV.cpp


AM_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

fv_LDADD = $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)

josiah_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
josiah_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

#m1_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS) $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
#m1_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

corpustrainer_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
corpustrainer_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

samplerank_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
samplerank_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a
