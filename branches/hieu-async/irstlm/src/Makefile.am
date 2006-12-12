
METASOURCES = AUTO
lib_LIBRARIES = libirstlm.a
libirstlm_a_SOURCES = cmd.c cmd.h compile-lm.cpp dictionary.cpp dictionary.h\
	gzfilebuf.h htable.cpp htable.h index.h lmtable.cpp lmtable.h\
	mempool.cpp mempool.h ngramcache.cpp ngramcache.h n_gram.cpp n_gram.h\
	quantize-lm.cpp util.cpp util.h
AM_CXXFLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
