bin_PROGRAMS = moses 
moses_SOURCES = Main.cpp mbr.cpp IOStream.cpp TranslationAnalysis.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src

moses_LDADD = -L$(top_srcdir)/moses/src -lmoses
moses_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

