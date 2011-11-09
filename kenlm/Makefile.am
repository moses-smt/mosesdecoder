lib_LTLIBRARIES = libkenlm.la
bin_PROGRAMS = query build_binary

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES $(BOOST_CPPFLAGS)
libkenlm_la_SOURCES = \
  lm/bhiksha.cc \
	lm/binary_format.cc \
	lm/config.cc \
	lm/lm_exception.cc \
	lm/model.cc \
	lm/search_hashed.cc \
	lm/search_trie.cc \
	lm/quantize.cc \
	lm/read_arpa.cc \
	lm/trie.cc \
	lm/trie_sort.cc \
	lm/virtual_interface.cc \
	lm/vocab.cc \
	util/file.cc \
	util/murmur_hash.cc   \
	util/mmap.cc   \
	util/file_piece.cc   \
	util/ersatz_progress.cc   \
	util/exception.cc   \
	util/bit_packing.cc 

query_SOURCES = lm/ngram_query.cc
query_LDADD = libkenlm.la

build_binary_SOURCES = lm/build_binary.cc
build_binary_LDADD = libkenlm.la

