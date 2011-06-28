// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#ifndef MEMSCORE_H
#define MEMSCORE_H

#include <sstream>
#include <string>
#include <utility>

#include "config.h"

#ifndef HAVE_PTRDIFF_T
typedef long ptrdiff_t;
#endif

#ifdef __GNUC__
#define NORETURN __attribute__ ((noreturn))
#else
#define NORETURN
#endif

void usage() NORETURN;

typedef double Score;
typedef unsigned int Count;
typedef unsigned int Phrase;
typedef ptrdiff_t DataIndex;
typedef std::pair<Phrase,Phrase> PhrasePair;
typedef char *PhrasePairData;
typedef std::string String;
typedef std::istringstream IStringStream;

/* phrasetable.h */

class PhraseText;
class PhraseInfo;
class PhraseInfoList;
class PhraseAlignment;
class PhrasePairInfo;
class PhraseTable;

/* scorer.h */

class PhraseScorer;

/* statistic.h */

class PhraseStatistic;

/* IRSTLM */

class lmtable;
class ngram;

#endif
