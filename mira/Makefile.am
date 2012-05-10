bin_PROGRAMS = mira
mira_SOURCES = \
    Decoder.cpp \
    Main.cpp  \
    MiraOptimiser.cpp \
    Perceptron.cpp \
    Hildreth.cpp


AM_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src -I$(top_srcdir)/moses-chart/src $(BOOST_CPPFLAGS)


mira_LDADD = $(top_builddir)/moses/src/libmoses.la -L$(top_srcdir)/moses/src  -lmoses -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIBS) $(BOOST_PROGRAM_OPTIONS_LDFLAGS) $(BOOST_PROGRAM_OPTIONS_LIBS)  $(BOOST_MPI_LIBS) 
mira_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la
