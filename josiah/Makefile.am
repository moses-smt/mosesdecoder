lib_LIBRARIES = libjosiah.a
bin_PROGRAMS = josiah samplerank unittest truncate


libjosiah_a_SOURCES = \
	AnnealingSchedule.cpp \
  Bleu.cpp \
  Derivation.cpp \
	Decoder.cpp \
  Dependency.cpp \
  DiscriminativeLMFeature.cpp \
  DistortionPenaltyFeature.cpp \
  Gain.cpp \
  Gibbler.cpp \
  GibblerMaxDerivDecoder.cpp \
  GibblerMaxTransDecoder.cpp \
  GibbsOperator.cpp \
	InputSource.cpp \
  LanguageModelFeature.cpp \
  LexicalReorderingFeature.cpp \
  MetaFeature.cpp \
  MpiDebug.cpp \
	Optimizer.cpp \
  ParenthesisFeature.cpp \
  PhraseBoundaryFeature.cpp \
  PhraseFeature.cpp \
  PhrasePairFeature.cpp \
  PosProjectionFeature.cpp \
  Pos.cpp \
  ReorderingFeature.cpp \
  SampleCollector.cpp \
  Sampler.cpp \
  Selector.cpp \
  SingleStateFeature.cpp \
  SourceToTargetRatio.cpp \
  StatelessFeature.cpp \
	TrainingSource.cpp \
	TranslationDelta.cpp \
  Utils.cpp \
  WeightManager.cpp \
  WordPenaltyFeature.cpp

josiah_SOURCES = \
  GibblerAnnealedExpectedLossTrainer.cpp \
  GibblerExpectedLossTraining.cpp \
  Josiah.cpp


samplerank_SOURCES = \
    SampleRank.cpp \
    SampleRankSelector.cpp \
    OnlineLearner.cpp \
    OnlineTrainingCorpus.cpp

unittest_SOURCES = \
  OnlineTrainingCorpus.cpp \
  Test.cpp \
  TestBleu.cpp  \
  TestOnlineTrainingCorpus.cpp

truncate_SOURCES = \
  Truncate.cpp
   
AM_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)


josiah_LDADD = -L$(top_srcdir)/josiah -ljosiah $(top_builddir)/moses/src/libmoses.la -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt @KENLM_LDFLAGS@  $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIBS) $(BOOST_PROGRAM_OPTIONS_LDFLAGS) $(BOOST_PROGRAM_OPTIONS_LIBS) $(BOOST_MPI_LIBS)
josiah_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la $(top_srcdir)/josiah/libjosiah.a 

samplerank_LDADD =  -L$(top_srcdir)/josiah -ljosiah $(top_builddir)/moses/src/libmoses.la -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt @KENLM_LDFLAGS@   $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIBS) $(BOOST_PROGRAM_OPTIONS_LDFLAGS) $(BOOST_PROGRAM_OPTIONS_LIBS) $(BOOST_MPI_LIBS)
samplerank_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la $(top_srcdir)/josiah/libjosiah.a 

unittest_CPPFLAGS = $(AM_CPPFLAGS) -DBOOST_TEST_DYN_LINK
unittest_LDADD =  -L$(top_srcdir)/josiah -ljosiah $(top_builddir)/moses/src/libmoses.la -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt @KENLM_LDFLAGS@   $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIBS) $(BOOST_PROGRAM_OPTIONS_LDFLAGS) $(BOOST_PROGRAM_OPTIONS_LIBS) $(BOOST_MPI_LIBS) -lboost_unit_test_framework
unittest_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la $(top_srcdir)/josiah/libjosiah.a 

truncate_LDADD = -L$(top_srcdir)/josiah -ljosiah $(top_builddir)/moses/src/libmoses.la -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt @KENLM_LDFLAGS@    $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIBS) $(BOOST_PROGRAM_OPTIONS_LDFLAGS) $(BOOST_PROGRAM_OPTIONS_LIBS)
truncate_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la $(top_srcdir)/josiah/libjosiah.a 

