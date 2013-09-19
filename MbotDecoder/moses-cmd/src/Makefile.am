bin_PROGRAMS = moses lmbrgrid checkplf

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src  $(BOOST_CPPFLAGS)

checkplf_SOURCES = checkplf.cpp
checkplf_LDADD = $(top_builddir)/moses/src/libmoses.la  -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) 

moses_SOURCES = Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp LatticeMBR.cpp
moses_LDADD = $(top_builddir)/moses/src/libmoses.la  -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) 


lmbrgrid_SOURCES = LatticeMBRGrid.cpp LatticeMBR.cpp IOWrapper.cpp
lmbrgrid_LDADD = $(top_builddir)/moses/src/libmoses.la -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) 
