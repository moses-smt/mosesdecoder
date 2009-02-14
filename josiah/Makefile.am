bin_PROGRAMS = josiah
josiah_SOURCES = Derivation.cpp Decoder.cpp Josiah.cpp \
	GainFunction.cpp \
	MBRDecoder.cpp \
        Gibbler.cpp \
        GibblerExpectedLossTraining.cpp \
        GibblerMaxTransDecoder.cpp \
        GibbsOperator.cpp \
	TranslationDelta.cpp \
	SentenceBleu.cpp

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

#josiah_LDFLAGS = -static
josiah_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS)  $(BOOST_PROGRAM_OPTIONS_LIB)
josiah_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

