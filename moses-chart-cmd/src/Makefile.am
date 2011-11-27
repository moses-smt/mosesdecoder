bin_PROGRAMS = moses_chart
moses_chart_SOURCES = Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

moses_chart_LDADD = -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) 
moses_chart_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la $(top_srcdir)/OnDiskPt/src/libOnDiskPt.a




