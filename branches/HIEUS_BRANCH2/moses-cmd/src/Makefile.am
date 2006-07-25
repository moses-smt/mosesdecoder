bin_PROGRAMS = moses 
moses_SOURCES = Main.cpp IOCommandLine.cpp  IOFile.cpp 
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES

moses_LDADD = -lmoses
moses_DEPENDENCIES = $(top_srcdir)/../moses/src/libmoses.a

if BUILD_MYSQL_SUPPORT
moses_SOURCES += IOMySQL.cpp ThreadMySQL.cpp
endif

