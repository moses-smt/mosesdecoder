bin_PROGRAMS = CreateBerkeleyPt 
CreateBerkeleyPt_SOURCES = Main.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src

CreateBerkeleyPt_LDADD = -L$(top_srcdir)/BerkeleyPt/src -L$(top_srcdir)/moses/src -lBerkeleyPt -lmoses
CreateBerkeleyPt_DEPENDENCIES = $(top_srcdir)/BerkeleyPt/src/libBerkeleyPt.a  $(top_srcdir)/moses/src/libmoses.a



