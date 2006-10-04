lib_LIBRARIES = libirstlm.a

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
	util.h

bin_PROGRAMS = compile-lm quantize-lm

AM_LDFLAGS=-L.
LIBS=-lirstlm

compile_lm_SOURCES = compile-lm.cpp
quantize_lm_SOURCES = quantize-lm.cpp

