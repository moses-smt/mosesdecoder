bin_PROGRAMS = processPhraseTable processLexicalTable queryLexicalTable queryPhraseTable

processPhraseTable_SOURCES = GenerateTuples.cpp  processPhraseTable.cpp
processLexicalTable_SOURCES = processLexicalTable.cpp
queryLexicalTable_SOURCES = queryLexicalTable.cpp
queryPhraseTable_SOURCES = queryPhraseTable.cpp

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

processPhraseTable_LDADD = $(top_builddir)/moses/src/libmoses.la -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS)

processLexicalTable_LDADD = $(top_builddir)/moses/src/libmoses.la  -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS)

queryLexicalTable_LDADD = $(top_builddir)/moses/src/libmoses.la  -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS)

queryPhraseTable_LDADD = $(top_builddir)/moses/src/libmoses.la  -L$(top_srcdir)/moses/src -L$(top_srcdir)/OnDiskPt/src -lmoses -lOnDiskPt $(top_srcdir)/util/libkenutil.la $(top_srcdir)/lm/libkenlm.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) 
