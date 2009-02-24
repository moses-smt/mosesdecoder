bin_PROGRAMS = josiah m1
josiah_SOURCES = Derivation.cpp \
	AnnealingSchedule.cpp \
	Decoder.cpp \
	InputSource.cpp \
	TrainingSource.cpp \
	Josiah.cpp \
	Optimizer.cpp \
	GainFunction.cpp \
	MBRDecoder.cpp \
        Gibbler.cpp \
        GibblerExpectedLossTraining.cpp \
        GibblerMaxTransDecoder.cpp \
        GibbsOperator.cpp \
	Model1.cpp \
	TranslationDelta.cpp \
	SentenceBleu.cpp \
    FeatureFunction.cpp \
    Score.cpp

m1_SOURCES = Model1.cpp FeatureFunction.cpp m1.cpp

AM_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

josiah_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
josiah_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

m1_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS) $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB)
m1_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

