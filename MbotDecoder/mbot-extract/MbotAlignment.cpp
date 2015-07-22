/*
 *  PhraseAlignment.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 28/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <sstream>
#include <cstring>

#include "MbotAlignment.h"
#include "SafeGetline.h"
#include "tables-core.h"
#include "score.h"

#include <cstdlib>

using namespace std;


extern Vocabulary vcbT;
extern Vocabulary vcbS;

extern bool inverseFlag;

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
void MbotAlignment::create( char line[], int lineID, string whichSide )
{
  assert(phraseS.empty());
  assert(phraseT.empty());

  vector< string > token = tokenize( line );
  int item = 1;
  int numMbots = 0;
  vector<WORD_ID> mbotVec;
  vector< set <size_t> > compAlign;

  for (size_t j=0; j<token.size(); j++) {
    if (token[j] == "|||") {
      if ( item == 2 ) {
    	phraseS.push_back(mbotVec);
    	if ( inverseFlag ) {
    	  size_t numSrcSym = mbotVec.size()-1;
    	  compAlign.resize(numSrcSym);
    	  alignedToS.push_back(compAlign);
    	  // NIN: If inverse: No information about number of target tree fragments
    	  // for target side => copy size of source alignments for target alignments
    	  alignedToT.push_back(compAlign);
    	}
     	mbotVec.clear();
      }
      else if ( item == 3 ) {
    	phraseT.push_back(mbotVec);
    	if ( !inverseFlag ) {
    	  size_t numTgtSym = mbotVec.size()-1;
    	  compAlign.resize(numTgtSym);
    	  alignedToT.push_back(compAlign);
    	  // NIN: If direct: No information about number of target tree fragments
    	  // for source side => copy size of target alignments for source alignments
    	  alignedToS.push_back(compAlign);
    	}
    	mbotVec.clear();
      }
      item++;
    }
    else if (item == 2) { // source phrase
      if ( token[j] == "||" ) {
    	phraseS.push_back(mbotVec);
       	size_t numSrcSym = mbotVec.size()-1;
       	compAlign.resize(numSrcSym);
       	alignedToS.push_back(compAlign);
       	// NIN: No information about number of target tree fragments...
       	// simply copy the target alignments for source alignments
       	alignedToT.push_back(compAlign);
       	mbotVec.clear();
    	continue;
      }
      mbotVec.push_back( vcbS.storeIfNew( token[j] ));
    }
    else if (item == 3) { // target phrase
      if ( token[j] == "||" ) {
    	phraseT.push_back(mbotVec);
    	size_t numTgtSym = mbotVec.size()-1;
    	compAlign.resize(numTgtSym);
       	// NIN: No information in about number of target tree fragments...
       	// simply copy the target alignments for source alignments
    	alignedToT.push_back(compAlign);
     	alignedToS.push_back(compAlign);
    	mbotVec.clear();
    	continue;
      }
      mbotVec.push_back( vcbT.storeIfNew( token[j] ));
    } 
    else if (item == 4) { // alignment
      while ( (j < token.size()) && (token[j] != "|||") ) {
      	if (token[j] == "||") {
      	  numMbots++;
      	  j++;
      	  continue;
      	}
    	int s,t;
    	sscanf(token[j].c_str(), "%d-%d", &s, &t);
    	if ( inverseFlag ) {
      	  alignedToS[numMbots][s].insert(t);
      	  if ( t >= alignedToT[numMbots].size() ) {
      		alignedToT[numMbots].resize(t+1);
      	  }
      	  alignedToT[numMbots][t].insert(s);
    	}
    	else if ( !inverseFlag ) {
    	  alignedToT[numMbots][t].insert(s);
    	  if ( s >= alignedToS[numMbots].size() ) {
    		alignedToS[numMbots].resize(s+1);
    	  }
    	  alignedToS[numMbots][s].insert(t);
    	}
        ++j;
      }
      --j;
    } 
    else if (item == 5) { // count
      sscanf(token[j].c_str(), "%f", &count);
    }
  }

  if (item < 4 || item > 6) {
    cerr << "ERROR: faulty line" << lineID << ": " << line << endl;
  }
}

void MbotAlignment::clear()
{
  phraseS.clear();
  phraseT.clear();
  alignedToT.clear();
  alignedToS.clear();
}

// check if two word alignments between a phrase pair are the same
bool MbotAlignment::equals( const MbotAlignment& other )
{
  if (this == &other) return true;
  if (other.GetTarget() != GetTarget()) return false;
  if (other.GetSource() != GetSource()) return false;
  if (other.alignedToT != alignedToT) return false;
  if (other.alignedToS != alignedToS) return false;
  return true;
}


 int MbotAlignment::Compare(const MbotAlignment &other) const
{
  if (this == &other) // comparing with itself
    return 0;

  if (GetTarget() != other.GetTarget())
    return ( GetTarget() < other.GetTarget() ) ? -1 : +1;

  if (GetSource() != other.GetSource())
    return ( GetSource() < other.GetSource() ) ? -1 : +1;

  // loop over all words in each target tree fragment
  if ( !inverseFlag ) {
	for(size_t i=0; i<phraseT.size(); i++) {

	  for (size_t j=0; j<phraseT[i].size()-1; j++) {

		if (isNonTerminal( vcbT.getWord( phraseT[i][j] ) )) {
		  size_t thisAlign = *(alignedToT[i][j].begin());
		  size_t otherAlign = *(other.alignedToT[i][j].begin());

		  if (alignedToT[i][j].size() != 1 ||
			other.alignedToT[i][j].size() != 1 ||
			thisAlign != otherAlign) {
			  int ret = (thisAlign < otherAlign) ? -1 : +1;
			  return ret;
		  }
		}
	  }
	}
  }
  else {
	for(size_t i=0; i<phraseS.size(); i++) {

	  for (size_t j=0; j<phraseS[i].size()-1; j++) {

		if (isNonTerminal( vcbS.getWord( phraseS[i][j] ) )) {
		  size_t thisAlign = *(alignedToS[i][j].begin());
		  size_t otherAlign = *(other.alignedToS[i][j].begin());

		  if (alignedToS[i][j].size() != 1 ||
			  other.alignedToS[i][j].size() != 1 ||
			  thisAlign != otherAlign) {
			int ret = (thisAlign < otherAlign) ? -1 : +1;
			return ret;
		  }
		}
	  }
	}
  }
  return 0;
}


