bin_PROGRAMS = createOnDiskPt
createOnDiskPt_SOURCES = Main.cpp 
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src

createOnDiskPt_LDADD = -L$(top_srcdir)/moses/src -L$(top_srcdir)/moses-chart/src -L$(top_srcdir)/on-disk-phrase-dict/src -lon-disk-phrase-dict -lmoses -lmoses-chart
createOnDiskPt_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a $(top_srcdir)/moses-chart/src/libmoses-chart.a $(top_srcdir)/on-disk-phrase-dict/src/libon-disk-phrase-dict.a

