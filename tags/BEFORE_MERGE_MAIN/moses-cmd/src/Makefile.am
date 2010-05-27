bin_PROGRAMS = moses mosesmt lmbrgrid

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src  $(BOOST_CPPFLAGS)

moses_SOURCES = Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp LatticeMBR.cpp
moses_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt

mosesmt_SOURCES = MainMT.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp ThreadPool.cpp LatticeMBR.cpp
mosesmt_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt

lmbrgrid_SOURCES = LatticeMBRGrid.cpp LatticeMBR.cpp IOWrapper.cpp
lmbrgrid_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt
