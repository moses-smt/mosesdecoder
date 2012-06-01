/**
 * Common lossy counting phrase extraction functionality implementation.
 *
 * Note: The bulk of this unit is based on Philipp Koehn's code from
 * phrase-extract/extract.cpp.
 *
 * (C) Moses: http://www.statmt.org/moses/
 * (C) Ceslav Przywara, UFAL MFF UK, 2011
 *
 * $Id$
 */

#include <iostream>
#include <iomanip>
#include <sstream>

#include "phrase-extract.h"
#include "ISS.h"
// I'm using my own version of SafeGetline (without "using namespace std;"):
#include "SafeGetline.h"


#define LINE_MAX_LENGTH 60000


//////// Helping functions ////////

// For sorted output.
typedef std::pair<indexed_phrases_pair_t, PhrasePairsLossyCounter::frequency_t> output_pair_t;
typedef std::vector<output_pair_t> output_vector_t;

class PhraseComp {
    /** @var If true, sort by target phrase first. */
    bool _inverted;
    
    bool compareAlignments(const indexed_phrases_pair_t& a, const indexed_phrases_pair_t& b);

    int comparePhrases(const indexed_phrases_pair_t::phrase_t& a, const indexed_phrases_pair_t::phrase_t& b);
    
public:
    PhraseComp(bool inverted): _inverted(inverted) {}
    
    bool operator()(const output_pair_t& a, const output_pair_t& b);
};

void processSortedOutput(OutputProcessor& processor);

void processUnsortedOutput(OutputProcessor& processor);

void flushPhrasePair(OutputProcessor& processor, const indexed_phrases_pair_t& indexedPhrasePair, PhrasePairsLossyCounter::frequency_t frequency, int mode);


//////// Define variables declared as extern in the header /////////////////////
bool allModelsOutputFlag = false;

bool wordModel = false; // IBM word model.
REO_MODEL_TYPE wordType = REO_MSD;
bool phraseModel = false; // Std phrase-based model.
REO_MODEL_TYPE phraseType = REO_MSD;
bool hierModel = false; // Hierarchical model.
REO_MODEL_TYPE hierType = REO_MSD;

int maxPhraseLength = 0; // Eg. 7
bool translationFlag = true; // Generate extract and extract.inv
bool orientationFlag = false; // Ordering info needed?
bool sortedOutput = false; // Sort output?

LossyCountersVector lossyCounters;

#ifdef GET_COUNTS_ONLY
std::vector<size_t> phrasePairsCounters;
#endif


//////// Internal module variables /////////////////////////////////////////////

IndexedStringsStorage<word_index_t> strings;
IndexedStringsStorage<orientation_info_index_t> orientations;


//////// Untouched Philipp Koehn's code :) /////////////////////////////////////

REO_POS getOrientWordModel(SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
                           bool connectedLeftTop, bool connectedRightTop,
                           int startF, int endF, int startE, int endE, int countF, int zero, int unit,
                           bool (*ge)(int, int), bool (*lt)(int, int) )
{

  if( connectedLeftTop && !connectedRightTop)
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;
  if (!connectedLeftTop &&  connectedRightTop)
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit)
    connectedLeftTop = isAligned(sentence, indexF, startE-unit);
  for(int indexF=endF+2*unit; (*lt)(indexF,countF) && !connectedRightTop; indexF=indexF+unit)
    connectedRightTop = isAligned(sentence, indexF, startE-unit);
  if(connectedLeftTop && !connectedRightTop)
    return DRIGHT;
  else if(!connectedLeftTop && connectedRightTop)
    return DLEFT;
  return UNKNOWN;
}

// to be called with countF-1 instead of countF
REO_POS getOrientPhraseModel (SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
                              bool connectedLeftTop, bool connectedRightTop,
                              int startF, int endF, int startE, int endE, int countF, int zero, int unit,
                              bool (*ge)(int, int), bool (*lt)(int, int),
                              const HSentenceVertices & inBottomRight, const HSentenceVertices & inBottomLeft)
{

  HSentenceVertices::const_iterator it;

  if((connectedLeftTop && !connectedRightTop) ||
      //(startE == 0 && startF == 0) ||
      //(startE == sentence.target.size()-1 && startF == sentence.source.size()-1) ||
      ((it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
       it->second.find(startF-unit) != it->second.end()))
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;
  if((!connectedLeftTop &&  connectedRightTop) ||
      ((it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() && it->second.find(endF + unit) != it->second.end()))
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  connectedLeftTop = false;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit)
    if(connectedLeftTop = (it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
                          it->second.find(indexF) != it->second.end())
      return DRIGHT;
  connectedRightTop = false;
  for(int indexF=endF+2*unit; (*lt)(indexF, countF) && !connectedRightTop; indexF=indexF+unit)
    if(connectedRightTop = (it = inBottomLeft.find(startE - unit)) != inBottomRight.end() &&
                           it->second.find(indexF) != it->second.end())
      return DLEFT;
  return UNKNOWN;
}

// to be called with countF-1 instead of countF
REO_POS getOrientHierModel (SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
                            bool connectedLeftTop, bool connectedRightTop,
                            int startF, int endF, int startE, int endE, int countF, int zero, int unit,
                            bool (*ge)(int, int), bool (*lt)(int, int),
                            const HSentenceVertices & inBottomRight, const HSentenceVertices & inBottomLeft,
                            const HSentenceVertices & outBottomRight, const HSentenceVertices & outBottomLeft,
                            REO_POS phraseOrient)
{

  HSentenceVertices::const_iterator it;

  if(phraseOrient == LEFT ||
      (connectedLeftTop && !connectedRightTop) ||
      //    (startE == 0 && startF == 0) ||
      //(startE == sentence.target.size()-1 && startF == sentence.source.size()-1) ||
      ((it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
       it->second.find(startF-unit) != it->second.end()) ||
      ((it = outBottomRight.find(startE - unit)) != outBottomRight.end() &&
       it->second.find(startF-unit) != it->second.end()))
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;
  if(phraseOrient == RIGHT ||
      (!connectedLeftTop &&  connectedRightTop) ||
      ((it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() &&
       it->second.find(endF + unit) != it->second.end()) ||
      ((it = outBottomLeft.find(startE - unit)) != outBottomLeft.end() &&
       it->second.find(endF + unit) != it->second.end()))
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  if(phraseOrient != UNKNOWN)
    return phraseOrient;
  connectedLeftTop = false;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit) {
    if((connectedLeftTop = (it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
                           it->second.find(indexF) != it->second.end()) ||
        (connectedLeftTop = (it = outBottomRight.find(startE - unit)) != outBottomRight.end() &&
                            it->second.find(indexF) != it->second.end()))
      return DRIGHT;
  }
  connectedRightTop = false;
  for(int indexF=endF+2*unit; (*lt)(indexF, countF) && !connectedRightTop; indexF=indexF+unit) {
    if((connectedRightTop = (it = inBottomLeft.find(startE - unit)) != inBottomRight.end() &&
                            it->second.find(indexF) != it->second.end()) ||
        (connectedRightTop = (it = outBottomLeft.find(startE - unit)) != outBottomRight.end() &&
                             it->second.find(indexF) != it->second.end()))
      return DLEFT;
  }
  return UNKNOWN;
}

void insertVertex( HSentenceVertices & corners, int x, int y )
{
  std::set<int> tmp;
  tmp.insert(x);
  std::pair< HSentenceVertices::iterator, bool > ret = corners.insert( std::pair<int, std::set<int> > (y, tmp) );
  if(ret.second == false) {
    ret.first->second.insert(x);
  }
}

void insertPhraseVertices(
  HSentenceVertices & topLeft,
  HSentenceVertices & topRight,
  HSentenceVertices & bottomLeft,
  HSentenceVertices & bottomRight,
  int startF, int startE, int endF, int endE)
{

  insertVertex(topLeft, startF, startE);
  insertVertex(topRight, endF, startE);
  insertVertex(bottomLeft, startF, endE);
  insertVertex(bottomRight, endF, endE);
}

std::string getOrientString(REO_POS orient, REO_MODEL_TYPE modelType)
{
  switch(orient) {
  case LEFT:
    return "mono";
    break;
  case RIGHT:
    return "swap";
    break;
  case DRIGHT:
    return "dright";
    break;
  case DLEFT:
    return "dleft";
    break;
  case UNKNOWN:
    switch(modelType) {
    case REO_MONO:
      return "nomono";
      break;
    case REO_MSD:
      return "other";
      break;
    case REO_MSLR:
      return "dright";
      break;
    }
    break;
  }
}

bool ge(int first, int second)
{
  return first >= second;
}

bool le(int first, int second)
{
  return first <= second;
}

bool lt(int first, int second)
{
  return first < second;
}

bool isAligned ( SentenceAlignment &sentence, int fi, int ei )
{
  if (ei == -1 && fi == -1)
    return true;
  if (ei <= -1 || fi <= -1)
    return false;
  if (ei == sentence.target.size() && fi == sentence.source.size())
    return true;
  if (ei >= sentence.target.size() || fi >= sentence.source.size())
    return false;
  for(int i=0; i<sentence.alignedToT[ei].size(); i++)
    if (sentence.alignedToT[ei][i] == fi)
      return true;
  return false;
}

//////// END OF untouched Philipp Koehn's code :) //////////////////////////////


/////// Slightly modified Philipp Koehn's code :) //////////////////////////////

void extract(SentenceAlignment &sentence) {

    int countE = sentence.target.size();
    int countF = sentence.source.size();

    HPhraseVector inboundPhrases;

    HSentenceVertices inTopLeft;
    HSentenceVertices inTopRight;
    HSentenceVertices inBottomLeft;
    HSentenceVertices inBottomRight;

    HSentenceVertices outTopLeft;
    HSentenceVertices outTopRight;
    HSentenceVertices outBottomLeft;
    HSentenceVertices outBottomRight;

    HSentenceVertices::const_iterator it;

    bool relaxLimit = hierModel;
    bool buildExtraStructure = phraseModel || hierModel;

    // check alignments for target phrase startE...endE
    // loop over extracted phrases which are compatible with the word-alignments
    for (int startE = 0; startE < countE; startE++) {
        for (
                int endE = startE;
                ((endE < countE) && (relaxLimit || (endE < (startE + maxPhraseLength))));
                endE++
            ) {

            int minF = 9999;
            int maxF = -1;
            std::vector< int > usedF = sentence.alignedCountS;

            for (int ei = startE; ei <= endE; ei++) {
                for (int i = 0; i < sentence.alignedToT[ei].size(); i++) {
                    int fi = sentence.alignedToT[ei][i];
                    if (fi < minF) {
                        minF = fi;
                    }
                    if (fi > maxF) {
                        maxF = fi;
                    }
                    usedF[ fi ]--;
                }
            }

        if (maxF >= 0 && // aligned to any source words at all
            (relaxLimit || maxF-minF < maxPhraseLength)) { // source phrase within limits

            // check if source words are aligned to out of bound target words
            bool out_of_bounds = false;

            for (int fi=minF; fi<=maxF && !out_of_bounds; fi++) {
                if (usedF[fi]>0) {
                    // cout << "ouf of bounds: " << fi << "\n";
                    out_of_bounds = true;
                }
            }

            // cout << "doing if for ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n";
            if (!out_of_bounds) {
                // start point of source phrase may retreat over unaligned
                for (int startF=minF;
                        (startF>=0 &&
                        (relaxLimit || startF>maxF-maxPhraseLength) && // within length limit
                        (startF==minF || sentence.alignedCountS[startF]==0)); // unaligned
                        startF--
                    )
                    // end point of source phrase may advance over unaligned
                    for (int endF=maxF;
                            (endF<countF &&
                            (relaxLimit || endF<startF+maxPhraseLength) && // within length limit
                            (endF==maxF || sentence.alignedCountS[endF]==0)); // unaligned
                            endF++
                        ) { // at this point we have extracted a phrase
                        if (buildExtraStructure) { // phrase || hier
                            if (endE-startE < maxPhraseLength && endF-startF < maxPhraseLength) { // within limit
                                inboundPhrases.push_back(
                                    HPhrase(HPhraseVertex(startF,startE), HPhraseVertex(endF,endE))
                                );
                                insertPhraseVertices(
                                    inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                                    startF, startE, endF, endE
                                );
                            } else {
                                insertPhraseVertices(
                                    outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                                    startF, startE, endF, endE
                                );
                            }
                        } else {
                            std::string orientationInfo = "";
                            if (orientationFlag && wordModel) { // Added orientationFlag check.
                                REO_POS wordPrevOrient, wordNextOrient;
                                bool connectedLeftTopP  = isAligned( sentence, startF-1, startE-1 );
                                bool connectedRightTopP = isAligned( sentence, endF+1,   startE-1 );
                                bool connectedLeftTopN  = isAligned( sentence, endF+1, endE+1 );
                                bool connectedRightTopN = isAligned( sentence, startF-1,   endE+1 );
                                wordPrevOrient = getOrientWordModel(sentence, wordType, connectedLeftTopP, connectedRightTopP, startF, endF, startE, endE, countF, 0, 1, &ge, &lt);
                                wordNextOrient = getOrientWordModel(sentence, wordType, connectedLeftTopN, connectedRightTopN, endF, startF, endE, startE, 0, countF, -1, &lt, &ge);
                                orientationInfo += getOrientString(wordPrevOrient, wordType) + " " + getOrientString(wordNextOrient, wordType);
                            }
                            addPhrase(sentence, startE, endE, startF, endF, orientationInfo);
                        }
                    }
                }
            }
        }
    } // end of main for loop

    if (buildExtraStructure) { // phrase || hier
        std::string orientationInfo = "";
        REO_POS wordPrevOrient, wordNextOrient, phrasePrevOrient, phraseNextOrient, hierPrevOrient, hierNextOrient;

        for (int i = 0; i < inboundPhrases.size(); i++) {
            int startF = inboundPhrases[i].first.first;
            int startE = inboundPhrases[i].first.second;
            int endF = inboundPhrases[i].second.first;
            int endE = inboundPhrases[i].second.second;

            if ( orientationFlag ) { // Added orientationFlag check.

                bool connectedLeftTopP  = isAligned( sentence, startF-1, startE-1 );
                bool connectedRightTopP = isAligned( sentence, endF+1,   startE-1 );
                bool connectedLeftTopN  = isAligned( sentence, endF+1, endE+1 );
                bool connectedRightTopN = isAligned( sentence, startF-1,   endE+1 );

                if (wordModel) {
                    wordPrevOrient = getOrientWordModel(sentence, wordType,
                                                connectedLeftTopP, connectedRightTopP,
                                                startF, endF, startE, endE, countF, 0, 1,
                                                &ge, &lt);

                    wordNextOrient = getOrientWordModel(sentence, wordType,
                                                connectedLeftTopN, connectedRightTopN,
                                                endF, startF, endE, startE, 0, countF, -1,
                                                &lt, &ge);
                }
                if (phraseModel) {
                    phrasePrevOrient = getOrientPhraseModel(sentence, phraseType,
                                                    connectedLeftTopP, connectedRightTopP,
                                                    startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft);
                    phraseNextOrient = getOrientPhraseModel(sentence, phraseType,
                                                    connectedLeftTopN, connectedRightTopN,
                                                    endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight);
                } else {
                    phrasePrevOrient = phraseNextOrient = UNKNOWN;
                }
                if(hierModel) {
                    hierPrevOrient = getOrientHierModel(sentence, hierType,
                                                connectedLeftTopP, connectedRightTopP,
                                                startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft, outBottomRight, outBottomLeft, phrasePrevOrient);
                    hierNextOrient = getOrientHierModel(sentence, hierType,
                                                connectedLeftTopN, connectedRightTopN,
                                                endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight, outBottomLeft, outBottomRight, phraseNextOrient);
                }

                orientationInfo = ((wordModel)? getOrientString(wordPrevOrient, wordType) + " " + getOrientString(wordNextOrient, wordType) : "") + " | " +
                            ((phraseModel)? getOrientString(phrasePrevOrient, phraseType) + " " + getOrientString(phraseNextOrient, phraseType) : "") + " | " +
                            ((hierModel)? getOrientString(hierPrevOrient, hierType) + " " + getOrientString(hierNextOrient, hierType) : "");
            }
            
            addPhrase(sentence, startE, endE, startF, endF, orientationInfo);
            
        } // end of for loop through inbound phrases

    } // end if buildExtraStructure

} // end of extract()


/**
 * @param sentence
 * @param startE
 * @param endE
 * @param startF
 * @param endF
 * @param orientationInfo
 */
void addPhrase(SentenceAlignment &sentence, int startE, int endE, int startF, int endF, std::string &orientationInfo) {

#ifdef GET_COUNTS_ONLY
    // Just get the length of phrase pair (which is now defined as maximum of the two).
    phrasePairsCounters[std::max(endF - startF, endE - startE) + 1] += 1; // Don't forget +1 (span is inclusive)!
#else
    alignment_t alignment;

    // alignment
    for (int ei = startE; ei <= endE; ++ei) {
        for (int i = 0; i < sentence.alignedToT[ei].size(); ++i) {
            int fi = sentence.alignedToT[ei][i];
            alignment.push_back(alignment_t::value_type(fi-startF, ei-startE));
        }
    }

    indexed_phrases_pair_t::phrase_t srcPhraseIndices, tgtPhraseIndices;

    // source phrase
    for (int fi = startF; fi <= endF; ++fi) {
        srcPhraseIndices.push_back(strings.put(sentence.source[fi].c_str()));
    }

    // target phrase
    for (int ei = startE; ei <= endE; ++ei) {
        tgtPhraseIndices.push_back(strings.put(sentence.target[ei].c_str()));
    }

    // TODO: Allow for switching between min and max here.
    size_t idx = std::max(srcPhraseIndices.size(), tgtPhraseIndices.size());

    // Add phrase pair.
    lossyCounters[idx]->lossyCounter.add(indexed_phrases_pair_t(srcPhraseIndices, tgtPhraseIndices, orientations.put(orientationInfo.c_str()), alignment));
    //
    if ( lossyCounters[idx]->lossyCounter.aboutToPrune() ) {
        // Next addition will lead to pruning, inform:
        std::cerr << 'P' << idx << std::flush;
    }
#endif
} // end of addPhrase()


/////// Lossy Counting related code ////////////////////////////////////////////

void readInput(std::istream& eFile, std::istream& fFile, std::istream& aFile) {

    // Note: moved out of the loop.
    char englishString[LINE_MAX_LENGTH];
    char foreignString[LINE_MAX_LENGTH];
    char alignmentString[LINE_MAX_LENGTH];

    int i = 0;

    while(true) {
        // Report progress?
        if (++i%10000 == 0) std::cerr << "." << std::flush;

        SAFE_GETLINE(eFile, englishString, LINE_MAX_LENGTH, '\n', __FILE__);
        if (eFile.eof()) break;
        SAFE_GETLINE(fFile, foreignString, LINE_MAX_LENGTH, '\n', __FILE__);
        SAFE_GETLINE(aFile, alignmentString, LINE_MAX_LENGTH, '\n', __FILE__);

        SentenceAlignment sentence;

        if (sentence.create(englishString, foreignString, alignmentString, i)) {
            extract(sentence);
        }
    }

}


void processOutput(OutputProcessor& processor) {
    if ( sortedOutput ) {
        processSortedOutput(processor);
    }
    else {
        processUnsortedOutput(processor);
    }
}


bool PhraseComp::operator()(const output_pair_t& a, const output_pair_t& b) {

    int cmp = _inverted ? comparePhrases(a.first.tgtPhrase(), b.first.tgtPhrase()) : comparePhrases(a.first.srcPhrase(), b.first.srcPhrase());

    if ( cmp == 0 ) {
        // First part of pairs matches, compare the second part.
        cmp = _inverted ? comparePhrases(a.first.srcPhrase(), b.first.srcPhrase()) : comparePhrases(a.first.tgtPhrase(), b.first.tgtPhrase());

        if ( cmp == 0 ) {
            // Also second part matches, compare alignments.
            return compareAlignments(a.first, b.first);
        }
        else {
            return cmp < 0;
        }
    }
    else {
        return cmp < 0;
    }
    
}


bool PhraseComp::compareAlignments(const indexed_phrases_pair_t& a, const indexed_phrases_pair_t& b) {

    size_t aSize = a.alignmentLength();
    size_t bSize = b.alignmentLength();
    size_t min = std::min(aSize, bSize);
    const indexed_phrases_pair_t::alignment_point_t * aAlignment = a.alignmentData();
    const indexed_phrases_pair_t::alignment_point_t * bAlignment = b.alignmentData();

    int cmp = 0;
    for ( size_t i = 0; i < min; ++i ) {
        // Important: alignments have to be eventually inverted as well!
        if ( _inverted ) {
            // Inverted = compare TGT phrase alignment points first.
            cmp = memcmp(aAlignment + i*2 + 1, bAlignment + i*2 + 1, sizeof(indexed_phrases_pair_t::alignment_point_t));
        }
        else{
            // NOT inverted = compare SRC phrase alignment points first.
            cmp = memcmp(aAlignment+ i*2, bAlignment + i*2, sizeof(indexed_phrases_pair_t::alignment_point_t));
        }
        if ( cmp == 0 ) {
            if ( _inverted ) {
                // Inverted = compare SRC phrase alignment points second.
                cmp = memcmp(aAlignment + i*2, bAlignment + i*2, sizeof(indexed_phrases_pair_t::alignment_point_t));
            }
            else{
                // NOT inverted = compare TGT phrase alignment points second.
                cmp = memcmp(aAlignment + i*2 + 1, bAlignment + i*2 + 1, sizeof(indexed_phrases_pair_t::alignment_point_t));
            }
            if ( cmp != 0 ) {
                return cmp < 0;
            } // Otherwise continue looping.
        }
        else {
            return cmp < 0;
        }
    }
    
    // Note: LC_ALL=C GNU sort treats shorter item as lesser than longer one.
    return (cmp == 0) ? (aSize < bSize) : (cmp < 0);

}


int PhraseComp::comparePhrases(const indexed_phrases_pair_t::phrase_t& a, const indexed_phrases_pair_t::phrase_t& b) {

    size_t aSize = a.size();
    size_t bSize = b.size();
    size_t min = std::min(aSize, bSize);
    int cmp = 0;

    for ( size_t i = 0; i < min; ++i ) {
        cmp = strcmp(strings.get(a[i]), strings.get(b[i]));
        if ( cmp != 0 ) {
            return cmp;
        }
    }

    if ( aSize == bSize ) {
        return 0;
    }

    if ( aSize < bSize ) {
        return strcmp("|||", strings.get(b[min]));
    }
    else {
        return strcmp(strings.get(a[min]), "|||");
    }

}


void processSortedOutput(OutputProcessor& processor) {

    output_vector_t output;

    LossyCountersVector::value_type current = NULL, prev = NULL;

    for ( size_t i = 1; i < lossyCounters.size(); ++i ) { // Intentionally skip 0.
        current = lossyCounters[i];
        if ( current != prev ) {
            PhrasePairsLossyCounter& lossyCounter = current->lossyCounter;
            for ( PhrasePairsLossyCounter::erasing_iterator phraseIter = lossyCounter.beginErase(); phraseIter != lossyCounter.endErase(); ++phraseIter ) {
                // Store and...
                output.push_back(std::make_pair(phraseIter.item(), phraseIter.frequency()));
                // ...update counters.
                current->outputMass += phraseIter.frequency();
                current->outputSize += 1;
            }
            //
            prev = current;
            //delete current;
        }
    }

    // Sort by source phrase.
    std::sort(output.begin(), output.end(), PhraseComp(false));

    // Print.
    for ( output_vector_t::const_iterator iter = output.begin(); iter != output.end(); ++iter ) {
        flushPhrasePair(processor, iter->first, iter->second, 1);
    }

    // Sort by target phrase.
    std::sort(output.begin(), output.end(), PhraseComp(true));

    // Print.
    for ( output_vector_t::const_iterator iter = output.begin(); iter != output.end(); ++iter ) {
        flushPhrasePair(processor, iter->first, iter->second, -1);
    }

}


void processUnsortedOutput(OutputProcessor& processor) {
    
    LossyCountersVector::value_type current = NULL, prev = NULL;

    for ( size_t i = 1; i < lossyCounters.size(); ++i ) { // Intentionally skip 0.

        current = lossyCounters[i];

        if ( current != prev ) {

            const PhrasePairsLossyCounter& lossyCounter = current->lossyCounter;

            for ( PhrasePairsLossyCounter::const_iterator phraseIter = lossyCounter.begin(); phraseIter != lossyCounter.end(); ++phraseIter ) {
                // Flush and...
                flushPhrasePair(processor, phraseIter.item(), phraseIter.frequency(), 0);
                // ...update counters.
                current->outputMass += phraseIter.frequency();
                current->outputSize += 1;
            }

            //
            prev = current;
        }
    }

}


void flushPhrasePair(OutputProcessor& processor, const indexed_phrases_pair_t& indexedPhrasePair, PhrasePairsLossyCounter::frequency_t frequency, int mode = 0) {

    const indexed_phrases_pair_t::phrase_t srcPhraseIndices = indexedPhrasePair.srcPhrase();
    const indexed_phrases_pair_t::phrase_t tgtPhraseIndices = indexedPhrasePair.tgtPhrase();

    std::string srcPhrase, tgtPhrase;

    for ( indexed_phrases_pair_t::phrase_t::const_iterator indexIter = srcPhraseIndices.begin(); indexIter != srcPhraseIndices.end(); ++indexIter ) {
        srcPhrase += std::string(strings.get(*indexIter)) + " ";
    }
    srcPhrase.resize(srcPhrase.size() - 1); // Trim the trailing " "

    for ( indexed_phrases_pair_t::phrase_t::const_iterator indexIter = tgtPhraseIndices.begin(); indexIter != tgtPhraseIndices.end(); ++indexIter ) {
        tgtPhrase += std::string(strings.get(*indexIter)) + " ";
    }
    tgtPhrase.resize(tgtPhrase.size() - 1); // Trim the trailing " "

    // Actual processing is done via call to functor:
    processor(srcPhrase, tgtPhrase, orientations.get(indexedPhrasePair.orientationInfo()), indexedPhrasePair.alignment(), frequency, mode);
}


void printStats(void) {

    // Total counters.
    size_t outputMass = 0, outputSize = 0, N = 0;

    const std::string hline = "####################################################################################################################";

    std::cerr << "Lossy Counting Phrase Extraction statistics:" << std::endl;

    // Print header: | 3 | 15 | 15 | 15 | 7 | 10 | 10 | 10 |
    std::cerr
        << hline << std::endl
        << "# length #      unique out #       total out #    total in (N) # out/in (%) #  pos. thr. #  neg. thr. #  max. err. #" << std::endl
        << hline << std::endl;

    LossyCountersVector::value_type current = NULL, prev = NULL;
    size_t from = 1, to = 1;

    for ( size_t i = 1; i <= lossyCounters.size(); ++i ) { // Intentionally skip 0, intentionally increment till == size().

        current = (i < lossyCounters.size()) ? lossyCounters[i] : NULL;

        if ( (current == NULL) || ((current != prev) && (prev != NULL)) ) {
            // Time to print.
            to = i-1;
            
            // Increment overall stats.
            outputMass += prev->outputMass;
            outputSize += prev->outputSize;
            N += prev->lossyCounter.count();

            // Print.
            if ( from == to ) {
                std::cerr << "# " << std::setw(6) << to << " # ";
            }
            else {
                std::stringstream strStr;
                strStr << from << "-" << to;
                std::cerr << "# " << std::setw(6) << strStr.str() << " # ";
            }
            // Print the rest of record.
            std::cerr
                    << std::setw(15) << prev->outputSize << " # "
                    << std::setw(15) << prev->outputMass << " # "
                    << std::setw(15) << prev->lossyCounter.count() << " # "
                    << std::setw(10) << std::setprecision(4) << (static_cast<double>(prev->outputMass) / static_cast<double>(prev->lossyCounter.count())) * 100 << " # "
                    << std::setw(10) << prev->lossyCounter.threshold(true) << " # "
                    << std::setw(10) << prev->lossyCounter.threshold() << " # "
                    << std::setw(10) << prev->lossyCounter.maxError() << " #"
                    << std::endl << hline << std::endl;

            from = i;
        }
        
        prev = current;

    }

    // Print summary:
    std::cerr
        << "#  TOTAL # "
        << std::setw(15) << outputSize << " # "
        << std::setw(15) << outputMass << " # "
        << std::setw(15) << N << " # "
        << std::setw(10) << std::setprecision(4) << (static_cast<double>(outputMass) / static_cast<double>(N)) * 100 << " #"
        << std::endl
        << "#############################################################################" << std::endl;

}
