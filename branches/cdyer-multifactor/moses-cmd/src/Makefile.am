bin_PROGRAMS = moses 
moses_SOURCES = Main.cpp IOCommandLine.cpp  IOFile.cpp TranslationAnalysis.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src

moses_LDADD = -L$(top_srcdir)/moses/src -lmoses
moses_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

