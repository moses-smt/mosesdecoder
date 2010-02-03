if WITH_THREADS
bin_PROGRAMS = moses mosesmt 
else
bin_PROGRAMS = moses
endif
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src  $(BOOST_CPPFLAGS)
moses_SOURCES = Main.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp LatticeMBR.cpp
moses_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)
moses_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a 

mosesmt_SOURCES = MainMT.cpp mbr.cpp IOWrapper.cpp TranslationAnalysis.cpp ThreadPool.cpp
mosesmt_LDADD = -L$(top_srcdir)/moses/src $(BOOST_LDFLAGS) -lmoses $(BOOST_THREAD_LIB)
mosesmt_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a 



