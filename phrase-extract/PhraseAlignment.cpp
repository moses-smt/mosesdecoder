/*
 *  PhraseAlignment.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 28/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <sstream>
#include "PhraseAlignment.h"
#include "SafeGetline.h"
#include "tables-core.h"
#include "score.h"

#include <cstdlib>

using namespace std;

namespace MosesTraining
{

extern Vocabulary vcbT;
extern Vocabulary vcbS;

extern bool hierarchicalFlag;

//! convert string to variable of type T. Used to reading floats, int etc from files
template<typename T>
inline T Scan(const std::string &input)
{
  std::stringstream stream(input);
  T ret;
  stream >> ret;
  return ret;
}


//! speeded up version of above
template<typename T>
inline void Scan(std::vector<T> &output, const std::vector< std::string > &input)
{
  output.resize(input.size());
  for (size_t i = 0 ; i < input.size() ; i++) {
    output[i] = Scan<T>( input[i] );
  }
}


inline void Tokenize(std::vector<std::string> &output
                     , const std::string& str
                     , const std::string& delimiters = " \t")
{
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    output.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}

// speeded up version of above
template<typename T>
inline void Tokenize( std::vector<T> &output
                      , const std::string &input
                      , const std::string& delimiters = " \t")
{
  std::vector<std::string> stringVector;
  Tokenize(stringVector, input, delimiters);
  return Scan<T>(output, stringVector );
}

// read in a phrase pair and store it
void PhraseAlignment::create( char line[], int lineID, bool includeSentenceIdFlag )
{
  assert(phraseS.empty());
  assert(phraseT.empty());
  treeFragment.clear();

  vector< string > token = tokenize( line );
  int item = 1;
  for (size_t j=0; j<token.size(); j++) {
    if (token[j] == "|||") item++;
    else if (item == 1) { // source phrase
      phraseS.push_back( vcbS.storeIfNew( token[j] ) );
    }

    else if (item == 2) { // target phrase
      phraseT.push_back( vcbT.storeIfNew( token[j] ) );
    } else if (item == 3) { // alignment
      int s,t;
      sscanf(token[j].c_str(), "%d-%d", &s, &t);
      if ((size_t)t >= phraseT.size() || (size_t)s >= phraseS.size()) {
        cerr << "WARNING: phrase pair " << lineID
             << " has alignment point (" << s << ", " << t
             << ") out of bounds (" << phraseS.size() << ", " << phraseT.size() << ")\n";
      } else {
        // first alignment point? -> initialize
        createAlignVec(phraseS.size(), phraseT.size());

        // add alignment point
        alignedToT[t].insert( s );
        alignedToS[s].insert( t );
      }
    } else if ( (item >= 4) && (token[j] == "Tree") ) { // check for information with a key field
      ++j;
      while ( (j < token.size() ) && (token[j] != "|||") ) {
        treeFragment.append(" ");
        treeFragment.append(token[j]);
        ++j;
      }
      --j;
    } else if (includeSentenceIdFlag && item == 4) { // optional sentence id
      sscanf(token[j].c_str(), "%d", &sentenceId);
    } else if (item + (includeSentenceIdFlag?-1:0) == 4) { // count
      sscanf(token[j].c_str(), "%f", &count);
    } else if (item + (includeSentenceIdFlag?-1:0) == 5) { // target syntax PCFG score
      float pcfgScore = std::atof(token[j].c_str());
      pcfgSum = pcfgScore * count;
    }
  }

  createAlignVec(phraseS.size(), phraseT.size());

  if (item + (includeSentenceIdFlag?-1:0) == 3) {
    count = 1.0;
  }
  if (item < 3 || item > 6) {
    cerr << "ERROR: faulty line " << lineID << ": " << line << endl;
  }
}

void PhraseAlignment::createAlignVec(size_t sourceSize, size_t targetSize)
{
  // in case of no align info. always need align info, even if blank
  if (alignedToT.size() == 0) {
    size_t numTgtSymbols = (hierarchicalFlag ? targetSize-1 : targetSize);
    alignedToT.resize(numTgtSymbols);
  }

  if (alignedToS.size() == 0) {
    size_t numSrcSymbols = (hierarchicalFlag ? sourceSize-1 : sourceSize);
    alignedToS.resize(numSrcSymbols);
  }
}

void PhraseAlignment::clear()
{
  phraseS.clear();
  phraseT.clear();
  alignedToT.clear();
  alignedToS.clear();
}

// check if two word alignments between a phrase pair are the same
bool PhraseAlignment::equals( const PhraseAlignment& other )
{
  if (this == &other) return true;
  if (other.GetTarget() != GetTarget()) return false;
  if (other.GetSource() != GetSource()) return false;
  if (other.alignedToT != alignedToT) return false;
  if (other.alignedToS != alignedToS) return false;
  return true;
}

// check if two word alignments between a phrase pairs "match"
// i.e. they do not differ in the alignment of non-termimals
bool PhraseAlignment::match( const PhraseAlignment& other )
{
  if (this == &other) return true;
  if (other.GetTarget() != GetTarget()) return false;
  if (other.GetSource() != GetSource()) return false;
  if (!hierarchicalFlag) return true;

  assert(phraseT.size() == alignedToT.size() + 1);
  assert(alignedToT.size() == other.alignedToT.size());

  // loop over all words (note: 0 = left hand side of rule)
  for(size_t i=0; i<phraseT.size()-1; i++) {
    if (isNonTerminal( vcbT.getWord( phraseT[i] ) )) {
      if (alignedToT[i].size() != 1 ||
          other.alignedToT[i].size() != 1 ||
          *(alignedToT[i].begin()) != *(other.alignedToT[i].begin()))
        return false;
    }
  }
  return true;
}

int PhraseAlignment::Compare(const PhraseAlignment &other) const
{
  if (this == &other) // comparing with itself
    return 0;

  if (GetTarget() != other.GetTarget())
    return ( GetTarget() < other.GetTarget() ) ? -1 : +1;

  if (GetSource() != other.GetSource())
    return ( GetSource() < other.GetSource() ) ? -1 : +1;

  if (!hierarchicalFlag)
    return 0;

  // loop over all words (note: 0 = left hand side of rule)
  for(size_t i=0; i<phraseT.size()-1; i++) {
    if (isNonTerminal( vcbT.getWord( phraseT[i] ) )) {
      size_t thisAlign = *(alignedToT[i].begin());
      size_t otherAlign = *(other.alignedToT[i].begin());

      if (alignedToT[i].size() != 1 ||
          other.alignedToT[i].size() != 1 ||
          thisAlign != otherAlign) {
        int ret = (thisAlign < otherAlign) ? -1 : +1;
        return ret;
      }
    }
  }
  return 0;

}

}

