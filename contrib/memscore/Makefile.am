# memscore - in-memory phrase scoring for Statistical Machine Translation
# Christian Hardmeier, FBK-irst, Trento, 2010
# $Id$

## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign
AM_CXXFLAGS = $(BOOST_CPPFLAGS) -Wall -ffast-math -ftrapping-math -fomit-frame-pointer

bin_PROGRAMS = memscore
memscore_SOURCES =	datastorage.h memscore.h phrasetable.h scorer.h scorer-impl.h statistic.h timestamp.h \
			phrasetable.cpp memscore.cpp scorer.cpp lexdecom.cpp lexdecom.h

if IRSTLM
memscore_SOURCES +=	phraselm.cpp phraselm.h
endif
if CHANNEL_SCORER
memscore_SOURCES +=	channel-scorer.cpp channel-scorer.h
endif


memscore_LDADD =	$(IRSTLM_LIBS) $(GSL_LIBS)
