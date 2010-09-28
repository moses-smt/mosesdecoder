bin_PROGRAMS = mira
mira_SOURCES = \
    Decoder.cpp \
    Main.cpp  \
    MiraOptimiser.cpp \
    Perceptron.cpp


AM_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src -I$(top_srcdir)/moses-chart/src $(BOOST_CPPFLAGS)


mira_LDADD = $(top_builddir)/moses/src/libmoses.la -L$(top_srcdir)/moses/src -L$(top_srcdir)/moses-chart/src -lmoses -lmoses-chart $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_MPI_LIB) -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt
