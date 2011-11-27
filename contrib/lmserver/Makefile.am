bin_PROGRAMS = lmserver lmserver-debug

lmserver_SOURCES = lmserver.c lmserver.h thread.c srilm.cc
lmserver_debug_SOURCES = $(lmserver_SOURCES)
lmserver_CPPFLAGS = -DNDEBUG
lmserver_LDADD = @DAEMON_OBJ@ 
lmserver_debug_LDADD = @DAEMON_OBJ@
lmserver_DEPENDENCIES = @DAEMON_OBJ@
lmserver_debug_DEPENDENCIES = @DAEMON_OBJ@

DIST_DIRS = examples
EXTRA_DIST = examples daemon.c

dist-hook:
	rm -rf $(distdir)/doc/.svn/
	rm -rf $(distdir)/scripts/.svn/
	rm -rf $(distdir)/t/.svn/
	rm -rf $(distdir)/t/lib/.svn/
