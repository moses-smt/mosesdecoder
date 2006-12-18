INCLUDES = -I$(top_srcdir)/irstlm/src -I$(top_srcdir)/moses/src \
	-I$(top_srcdir)/moses-cmd/src -I../../../srilm/include
METASOURCES = AUTO
bin_PROGRAMS = moses-cmd
moses_cmd_SOURCES = IOStream.cpp IOStream.h Main.cpp Main.h\
	TranslationAnalysis.cpp TranslationAnalysis.h
moses_cmd_LDADD = $(top_builddir)/moses/src/libmoses.a \
	$(top_builddir)/irstlm/src/libirstlm.a -L../../../boost/lib -lboost_filesystem-gcc -lz
AM_CXXFLAGS = -DLM_SRI -DLM_IRST -DLM_INTERNAL -DTRACE_ENABLE\
	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
