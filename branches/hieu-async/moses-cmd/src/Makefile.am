
METASOURCES = AUTO
bin_PROGRAMS = moses-cmd
moses_cmd_SOURCES = IOStream.cpp IOStream.h Main.cpp Main.h\
	TranslationAnalysis.cpp TranslationAnalysis.h
AM_CXXFLAGS = -DLM_SRI -DLM_IRST -DLM_INTERNAL -D_FILE_OFFSET_BITS=64\
	-D_LARGE_FILES -DTRACE_ENABLE
moses_cmd_LDADD = $(top_builddir)/moses/src/libmoses.a \
	$(top_builddir)/irstlm/src/libirstlm.a -lz

INCLUDES = -I$(top_srcdir)/irstlm/src -I$(top_srcdir)/moses/src
