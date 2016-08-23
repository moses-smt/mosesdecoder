/**
 * Common lossy counting phrase extraction functionality declaration.
 *
 * Note: The bulk of this unit is based on Philipp Koehn's code from
 * phrase-extract/extract.cpp.
 *
 * (C) Moses: http://www.statmt.org/moses/
 * (C) Ceslav Przywara, UFAL MFF UK, 2011
 *
 * $Id$
 */

#ifndef PHRASE_EXTRACT_H
#define	PHRASE_EXTRACT_H

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../phrase-extract/SentenceAlignment.h"

#include "typedefs.h"


//////// Types definitions /////////////////////////////////////////////////////

// HPhraseVertex represents a point in the alignment matrix
typedef std::pair<int, int> HPhraseVertex;

// Phrase represents a bi-phrase; each bi-phrase is defined by two points in the alignment matrix:
// bottom-left and top-right
typedef std::pair<HPhraseVertex, HPhraseVertex> HPhrase;

// HPhraseVector is a vector of HPhrases
typedef std::vector<HPhrase> HPhraseVector;

// SentenceVertices represents, from all extracted phrases, all vertices that have the same positioning
// The key of the map is the English index and the value is a set of the source ones
typedef std::map<int, std::set<int> > HSentenceVertices;

//
typedef std::pair<PhrasePairsLossyCounter::error_t, PhrasePairsLossyCounter::support_t> params_pair_t;
//
typedef std::vector<PhrasePairsLossyCounter *> PhrasePairsLossyCountersVector;

// MSD - monotone, swap, discontinuous.
enum REO_MODEL_TYPE {REO_MSD, REO_MSLR, REO_MONO};
enum REO_POS {LEFT, RIGHT, DLEFT, DRIGHT, UNKNOWN};

struct LossyCounterInstance {
    // Statistics not provided by the lossy counter must be computed during
    // phrases flushing (ie. when input processing is done):
    size_t outputMass; // unique * freq
    size_t outputSize; // unique
    //
    PhrasePairsLossyCounter lossyCounter;

    LossyCounterInstance(PhrasePairsLossyCounter::error_t error, PhrasePairsLossyCounter::support_t support): outputMass(0), outputSize(0), lossyCounter(error, support) {}
};

//
typedef std::vector<LossyCounterInstance *> LossyCountersVector;

struct OutputProcessor {
    virtual void operator() (const std::string& srcPhrase, const std::string& tgtPhrase, const std::string& orientationInfo, const alignment_t& alignment, const size_t frequency, int mode) = 0;
};


//////// Functions declarations ////////////////////////////////////////////////

//// Untouched ////
REO_POS getOrientWordModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
                           int, int, int, int, int, int, int,
                           bool (*)(int, int), bool (*)(int, int));

REO_POS getOrientPhraseModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
                             int, int, int, int, int, int, int,
                             bool (*)(int, int), bool (*)(int, int),
                             const HSentenceVertices &, const HSentenceVertices &);

REO_POS getOrientHierModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
                           int, int, int, int, int, int, int,
                           bool (*)(int, int), bool (*)(int, int),
                           const HSentenceVertices &, const HSentenceVertices &,
                           const HSentenceVertices &, const HSentenceVertices &,
                           REO_POS);

void insertVertex(HSentenceVertices &, int, int);
void insertPhraseVertices(HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, int, int, int, int);

std::string getOrientString(REO_POS, REO_MODEL_TYPE);

bool ge(int, int);
bool le(int, int);
bool lt(int, int);
bool isAligned (SentenceAlignment &, int, int);
void extract(SentenceAlignment &);

//// Modified ////
void addPhrase(SentenceAlignment &, int, int, int, int, std::string &);

//// Added ////
void readInput(std::istream& eFile, std::istream& fFile, std::istream& aFile);
void processOutput(OutputProcessor& processor);
void printStats(void);


//////// Extern variables //////////////////////////////////////////////////////

extern bool allModelsOutputFlag;

// Some default setting, I guess...
extern bool wordModel; // IBM word model.
extern REO_MODEL_TYPE wordType;
extern bool phraseModel; // Std phrase-based model.
extern REO_MODEL_TYPE phraseType;
extern bool hierModel; // Hierarchical model.
extern REO_MODEL_TYPE hierType;

extern int maxPhraseLength; // Eg. 7
extern bool translationFlag; // Generate extract and extract.inv
extern bool orientationFlag; // Ordering info needed?
extern bool sortedOutput; // Sort output?

extern LossyCountersVector lossyCounters;

#ifdef GET_COUNTS_ONLY
extern std::vector<size_t> phrasePairsCounters;
#endif

#endif	/* PHRASE_EXTRACT_H */
