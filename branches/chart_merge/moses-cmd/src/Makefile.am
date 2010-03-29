if WITH_THREADS
  bin_PROGRAMS = moses mosesmt lmbrgrid
else
  bin_PROGRAMS = moses lmbrgrid
endif

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src  $(BOOST_CPPFLAGS)

moses_SOURCES = Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp LatticeMBR.cpp
moses_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)

mosesmt_SOURCES = MainMT.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp ThreadPool.cpp LatticeMBR.cpp
mosesmt_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)

lmbrgrid_SOURCES = LatticeMBRGrid.cpp LatticeMBR.cpp IOWrapper.cpp
lmbrgrid_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)
