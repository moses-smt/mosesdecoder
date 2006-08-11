lib_LIBRARIES = libirstlm.a

libirstlm_a_SOURCES = \
	dictionary.cpp \
	htable.cpp \
	lmtable.cpp \
	mempool.cpp \
	ngram.cpp

library_includedir=$(includedir)
library_include_HEADERS = dictionary.h lmtable.h ngram.h

bin_PROGRAMS = compile-lm

AM_LDFLAGS=-L .
LIBS=-lirstlm

compile_lm_SOURCES = compile-lm.cpp

