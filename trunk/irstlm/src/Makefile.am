lib_LIBRARIES = libirstlm.a
AM_CPPFLAGS = -fPIC

libirstlm_a_SOURCES = \
	dictionary.cpp \
	htable.cpp \
	lmtable.cpp \
	mempool.cpp \
	n_gram.cpp \
	ngramcache.cpp \
	util.cpp

library_includedir=$(includedir)
library_include_HEADERS = \
        dictionary.h \
        htable.h \
        lmtable.h \
        mempool.h \
        n_gram.h \
        ngramcache.h \
	util.h \
	gzfilebuf.h 

bin_PROGRAMS = compile-lm quantize-lm

AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
AM_LDFLAGS=-L.
LIBS=-lirstlm -lz

compile_lm_SOURCES = compile-lm.cpp
quantize_lm_SOURCES = quantize-lm.cpp

