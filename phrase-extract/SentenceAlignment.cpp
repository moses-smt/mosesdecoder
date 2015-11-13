/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include "SentenceAlignment.h"

#include <map>
#include <set>
#include <string>

#include "tables-core.h"
#include "util/tokenize.hh"

using namespace std;

namespace MosesTraining
{

SentenceAlignment::~SentenceAlignment() {}

void addBoundaryWords(vector<string> &phrase)
{
  phrase.insert(phrase.begin(), "<s>");
  phrase.push_back("</s>");
}

bool SentenceAlignment::processTargetSentence(const char * targetString, int, bool boundaryRules)
{
  target = util::tokenize(targetString);
  if (boundaryRules)
    addBoundaryWords(target);
  return true;
}

bool SentenceAlignment::processSourceSentence(const char * sourceString, int, bool boundaryRules)
{
  source = util::tokenize(sourceString);
  if (boundaryRules)
    addBoundaryWords(source);
  return true;
}

bool SentenceAlignment::create(const char targetString[],
                               const char sourceString[],
                               const char alignmentString[],
                               const char weightString[],
                               int sentenceID, bool boundaryRules)
{
  using namespace std;
  this->sentenceID = sentenceID;
  this->weightString = std::string(weightString);

  // process sentence strings and store in target and source members.
  if (!processTargetSentence(targetString, sentenceID, boundaryRules)) {
    return false;
  }
  if (!processSourceSentence(sourceString, sentenceID, boundaryRules)) {
    return false;
  }

  // check if sentences are empty
  if (target.size() == 0 || source.size() == 0) {
    cerr << "no target (" << target.size() << ") or source (" << source.size() << ") words << end insentence " << sentenceID << endl;
    cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
    return false;
  }

  // prepare data structures for alignments
  for(size_t i=0; i<source.size(); i++) {
    alignedCountS.push_back( 0 );
  }
  for(size_t i=0; i<target.size(); i++) {
    vector< int > dummy;
    alignedToT.push_back( dummy );
  }

  // reading in alignments
  vector<string> alignmentSequence = util::tokenize( alignmentString );
  for(size_t i=0; i<alignmentSequence.size(); i++) {
    int s,t;
    // cout << "scaning " << alignmentSequence[i].c_str() << endl;
    if (! sscanf(alignmentSequence[i].c_str(), "%d-%d", &s, &t)) {
      cerr << "WARNING: " << alignmentSequence[i] << " is a bad alignment point in sentence " << sentenceID << endl;
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return false;
    }

    if (boundaryRules) {
      ++s;
      ++t;
    }

    // cout << "alignmentSequence[i] " << alignmentSequence[i] << " is " << s << ", " << t << endl;
    if ((size_t)t >= target.size() || (size_t)s >= source.size()) {
      cerr << "WARNING: sentence " << sentenceID << " has alignment point (" << s << ", " << t << ") out of bounds (" << source.size() << ", " << target.size() << ")\n";
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return false;
    }
    alignedToT[t].push_back( s );
    alignedCountS[s]++;
  }

  if (boundaryRules) {
    alignedToT[0].push_back(0);
    alignedCountS[0]++;

    alignedToT.back().push_back(alignedCountS.size() - 1);
    alignedCountS.back()++;

  }

  return true;
}

void SentenceAlignment::invertAlignment()
{
  alignedToS.resize(source.size());
  for (size_t targetPos = 0; targetPos < alignedToT.size(); ++targetPos) {
    const std::vector<int> &vec = alignedToT[targetPos];
    for (size_t i = 0; i < vec.size(); ++i) {
      int sourcePos = vec[i];
      alignedToS[sourcePos].push_back(targetPos);
    }

  }
}

}

