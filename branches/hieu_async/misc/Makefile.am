bin_PROGRAMS = processPhraseTable
processPhraseTable_SOURCES = GenerateTuples.cpp  processPhraseTable.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src

processPhraseTable_LDADD = -L$(top_srcdir)/moses/src -lmoses
processPhraseTable_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

