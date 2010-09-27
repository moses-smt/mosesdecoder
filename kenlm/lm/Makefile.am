lib_LIBRARIES = libkenlm.a
bin_PROGRAMS = query build_binary

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES $(BOOST_CPPFLAGS)
libkenlm_a_SOURCES = \
	virtual_interface.cc \
	ngram.cc \
	exception.cc \
	../util/string_piece.cc \
	../util/scoped.cc \
	../util/murmur_hash.cc   \
	../util/mmap.cc   \
	../util/file_piece.cc   \
	../util/ersatz_progress.cc   \
	../util/exception_util.cc   \
	../util/string_piece.cc

query_SOURCES = ngram_query.cc
query_DEPENDENCIES = libkenlm.a
query_LDADD = -L$(top_srcdir)/kenlm/lm -lkenlm

build_binary_SOURCES = ngram_build_binary.cc
build_binary_DEPENDENCIES = libkenlm.a
build_binary_LDADD = -L$(top_srcdir)/kenlm/lm -lkenlm

