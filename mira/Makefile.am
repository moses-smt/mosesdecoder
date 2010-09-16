bin_PROGRAMS = mira
mira_SOURCES = \
    Decoder.cpp \
    Main.cpp  \
    Perceptron.cpp


AM_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)


mira_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS)  $(BOOST_SERIALIZATION_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_MPI_LIB) -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt
