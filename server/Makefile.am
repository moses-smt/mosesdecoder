bin_PROGRAMS = mosesserver
mosesserver_SOURCES = mosesserver.cpp
mosesserver_CPPFLAGS = -W -Wall -I$(top_srcdir)/moses/src $(XMLRPC_C_CPPFLAGS) $(BOOST_CPPFLAGS)
mosesserver_LDADD = -L$(top_srcdir)/moses/src -lmoses $(BOOST_LDFLAGS) $(XMLRPC_C_LIBS) $(BOOST_THREAD_LIB)
mosesserver_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la
