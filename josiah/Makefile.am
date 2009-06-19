bin_PROGRAMS = josiah m1 corpustrainer
josiah_SOURCES = Derivation.cpp \
	AnnealingSchedule.cpp \
	Decoder.cpp \
    Dependency.cpp \
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
    Pos.cpp \
    SourceToTargetRatio.cpp \
	TranslationDelta.cpp \
	SentenceBleu.cpp \
    FeatureFunction.cpp \
    SearchRandom.cpp \
    QuenchingSchedule.cpp \
    GibblerAnnealedExpectedLossTrainer.cpp 


m1_SOURCES = Model1.cpp FeatureFunction.cpp m1.cpp

corpustrainer_SOURCES = Derivation.cpp \
        AnnealingSchedule.cpp \
        Decoder.cpp \
    Dependency.cpp \
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
    FeatureFunction.cpp \
    SearchRandom.cpp \
    QuenchingSchedule.cpp \
    CorpusSampler.cpp \
    CorpusTrainer.cpp 
   

AM_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

josiah_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
josiah_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

m1_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS) $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
m1_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

corpustrainer_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
corpustrainer_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a
