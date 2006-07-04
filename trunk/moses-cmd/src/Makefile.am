bin_PROGRAMS = moses
moses_SOURCES = Main.cpp IOCommandLine.cpp  IOFile.cpp 

# this really needs to be fixed
INCLUDES = -I../../src
moses_LDADD = -L../../src -lmoses

if BUILD_MYSQL_SUPPORT
moses_SOURCES += IOMySQL.cpp ThreadMySQL.cpp
endif

