bin_PROGRAMS = processPhraseTable processLexicalTable queryLexicalTable queryPhraseTable

processPhraseTable_SOURCES = GenerateTuples.cpp  processPhraseTable.cpp
processLexicalTable_SOURCES = processLexicalTable.cpp
queryLexicalTable_SOURCES = queryLexicalTable.cpp
queryPhraseTable_SOURCES = queryPhraseTable.cpp

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

processPhraseTable_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt @KENLM_LDFLAGS@

processLexicalTable_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt @KENLM_LDFLAGS@

queryLexicalTable_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt @KENLM_LDFLAGS@

queryPhraseTable_LDADD = $(top_builddir)/moses/src/libmoses.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt @KENLM_LDFLAGS@
