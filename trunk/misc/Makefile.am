bin_PROGRAMS = processPhraseTable processLexicalTable queryLexicalTable

processPhraseTable_SOURCES = GenerateTuples.cpp  processPhraseTable.cpp
processLexicalTable_SOURCES = processLexicalTable.cpp
queryLexicalTable_SOURCES    = queryLexicalTable.cpp

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src

processPhraseTable_LDADD = -L$(top_srcdir)/moses/src -lmoses
processPhraseTable_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

processLexicalTable_LDADD = -L$(top_srcdir)/moses/src -lmoses
processLexicalTable_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

queryLexicalTable_LDADD = -L$(top_srcdir)/moses/src -lmoses
queryLexicalTable_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a
