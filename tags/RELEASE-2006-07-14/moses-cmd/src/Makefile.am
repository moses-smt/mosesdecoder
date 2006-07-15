bin_PROGRAMS = moses processPhraseTable
moses_SOURCES = Main.cpp IOCommandLine.cpp  IOFile.cpp 

##MOSESLIB = -L$(top_srcdir)/../moses/src -lmoses
MOSESLIB = -lmoses

processPhraseTable_SOURCES = processPhraseTable.cpp
processPhraseTable_LDADD = $(MOSESLIB)

# this really needs to be fixed
### INCLUDES = -I../../src
moses_LDADD = $(MOSESLIB)

if BUILD_MYSQL_SUPPORT
moses_SOURCES += IOMySQL.cpp ThreadMySQL.cpp
endif

