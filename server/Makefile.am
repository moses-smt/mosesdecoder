bin_PROGRAMS = mosesserver
mosesserver_SOURCES = mosesserver.cpp
mosesserver_CPPFLAGS = -W -Wall -I$(top_srcdir)/moses/src $(XMLRPC_C_CPPFLAGS) $(BOOST_CPPFLAGS)
mosesserver_LDADD = -L$(top_srcdir)/moses/src -lmoses -L$(top_srcdir)/OnDiskPt/src -lOnDiskPt @KENLM_LDFLAGS@ $(BOOST_THREAD_LDFLAGS) $(XMLRPC_C_LIBS) $(BOOST_THREAD_LIBS) 
mosesserver_DEPENDENCIES = $(top_srcdir)/moses/src/libmoses.la $(top_srcdir)/OnDiskPt/src/libOnDiskPt.a @KENLM_DEPS@
