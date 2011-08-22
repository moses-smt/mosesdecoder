# eppex - epochal phrase table extraction for Statistical Machine Translation
# Ceslav Przywara, UFAL MFF UK, Prague, 2011
# $Id: $

# Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign
# Note: during development eppex has been compiled with -O6, but this flag
# gets overwritten by -O2 set by automake.
AM_CXXFLAGS = $(BOOST_CPPFLAGS) -Wall

bin_PROGRAMS = counter eppex

# Counter shares only some functionality of phrase-extract module.
counter_CXXFLAGS = -DGET_COUNTS_ONLY

# Uncomment to use std::tr1::unordered_map insteap of std::map in Lossy Counter implementation.
# This is NOT recommended in the moment (hashing function needs to be optimized).
#eppex_CXXFLAGS = -DUSE_UNORDERED_MAP

counter_SOURCES = ../phrase-extract/tables-core.h ../phrase-extract/SentenceAlignment.h config.h phrase-extract.h shared.h IndexedPhrasesPair.h LossyCounter.h \
	../phrase-extract/tables-core.cpp ../phrase-extract/SentenceAlignment.cpp phrase-extract.cpp shared.cpp counter.cpp

eppex_SOURCES = ../phrase-extract/tables-core.h ../phrase-extract/SentenceAlignment.h config.h phrase-extract.h shared.h IndexedPhrasesPair.h LossyCounter.h \
	../phrase-extract/tables-core.cpp ../phrase-extract/SentenceAlignment.cpp phrase-extract.cpp shared.cpp eppex.cpp

