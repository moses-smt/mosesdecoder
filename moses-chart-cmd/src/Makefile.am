bin_PROGRAMS = moses_chart
moses_chart_SOURCES = Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

moses_chart_LDADD = -L$(top_srcdir)/moses/src -L$(top_srcdir)/moses-chart/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lmoses-chart -lOnDiskPt $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) @KENLM_LDFLAGS@
moses_chart_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la $(top_srcdir)/moses-chart/src/libmoses-chart.a $(top_srcdir)/OnDiskPt/src/libOnDiskPt.a @KENLM_DEPS@




