bin_PROGRAMS = moses mosesmt 
moses_SOURCES = Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp
mosesmt_SOURCES = MainMT.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp ThreadPool.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src

moses_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_THREAD_LIB)
moses_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a 
mosesmt_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_THREAD_LIB)
mosesmt_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a 

