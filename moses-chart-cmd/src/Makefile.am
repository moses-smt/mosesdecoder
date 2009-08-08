bin_PROGRAMS = moses_chart
moses_chart_SOURCES = Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src

moses_chart_LDADD = -L$(top_srcdir)/moses/src -L$(top_srcdir)/moses-chart/src -L$(top_srcdir)/BerkeleyPt/src -lmoses -lmoses-chart -lBerkeleyPt
moses_chart_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a $(top_srcdir)/moses-chart/src/libmoses-chart.a  $(top_srcdir)/BerkeleyPt/src/libBerkeleyPt.a



