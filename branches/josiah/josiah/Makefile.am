bin_PROGRAMS = josiah
josiah_SOURCES = Decoder.cpp Josiah.cpp
AM_CPPFLAGS = -W -Wall -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DUSE_HYPO_POOL -I$(top_srcdir)/moses/src $(BOOST_CPPFLAGS)

josiah_LDADD = -L$(top_srcdir)/moses/src -lmoses
josiah_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.a

