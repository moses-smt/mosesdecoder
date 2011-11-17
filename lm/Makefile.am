lib_LTLIBRARIES = libkenlm.la
bin_PROGRAMS = query build_binary

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES $(BOOST_CPPFLAGS)
libkenlm_la_SOURCES = \
  bhiksha.cc \
	binary_format.cc \
	config.cc \
	lm_exception.cc \
	model.cc \
	search_hashed.cc \
	search_trie.cc \
	quantize.cc \
	read_arpa.cc \
	trie.cc \
	trie_sort.cc \
	virtual_interface.cc \
	vocab.cc

query_SOURCES = ngram_query.cc
query_LDADD = libkenlm.la $(top_srcdir)/util/libkenutil.la

build_binary_SOURCES = build_binary.cc
build_binary_LDADD = libkenlm.la $(top_srcdir)/util/libkenutil.la

