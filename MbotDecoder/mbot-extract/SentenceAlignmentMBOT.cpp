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

#include <map>
#include <set>
#include <string>
#include <cstdio>
#include <iostream>

#include "SentenceAlignmentMBOT.h"
#include "XmlTree.h"
#include "XmlException.h"
#include "tables-core.h"

using namespace std;


bool SentenceAlignmentMBOT::processTargetSentence(const char * targetString, int sentenceID)
{
  string targetStringCPP(targetString);
  try {
    ProcessAndStripXMLTags(targetStringCPP, targetTree,
                           m_targetLabelCollection,
                           m_targetTopLabelCollection,
                           false);
    } catch (const XmlException & e) {
    std::cerr << "WARNING: failed to process target sentence at line "
              << sentenceID << ": " << e.getMsg() << std::endl;
    return false;
    }
  target = tokenize(targetStringCPP.c_str());
  return true;
}


bool SentenceAlignmentMBOT::processSourceSentence(const char * sourceString, int sentenceID)
{
  if (!m_options.sourceSyntax) {
    source = tokenize(sourceString);
    return true;
  }

  string sourceStringCPP(sourceString);
  try {
    ProcessAndStripXMLTags(sourceStringCPP, sourceTree,
                           m_sourceLabelCollection ,
                           m_sourceTopLabelCollection,
                           false);
  } catch (const XmlException & e) {
    std::cerr << "WARNING: failed to process source sentence at line "
              << sentenceID << ": " << e.getMsg() << std::endl;
    return false;
  }
  source = tokenize(sourceStringCPP.c_str());
  return true;
}


bool SentenceAlignmentMBOT::create( char targetString[], char sourceString[], char alignmentString[], int sentenceID)
{
  using namespace std;
  this->sentenceID = sentenceID;

  // process sentence strings and store in target and source members.
  if (!processTargetSentence(targetString, sentenceID)) {
    return false;
  }
  if (!processSourceSentence(sourceString, sentenceID)) {
    return false;
    } 

  // check if sentences are empty
  if (target.size() == 0 || source.size() == 0) {
    cerr << "no target (" << target.size() << ") or source (" << source.size() << ") words << end in sentence " << sentenceID << endl;
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

  for(size_t i=0; i<target.size(); i++) {
    alignedCountT.push_back( 0 );
  }
  for(size_t i=0; i<source.size(); i++) {
    vector< int > dummy;
    alignedToS.push_back( dummy );
  }

  // reading in alignments
  vector<string> alignmentSequence = tokenize( alignmentString );
  for(size_t i=0; i<alignmentSequence.size(); i++) {
    int s,t;
    // cout << "scaning " << alignmentSequence[i].c_str() << endl;
    if (! sscanf(alignmentSequence[i].c_str(), "%d-%d", &s, &t)) {
      cerr << "WARNING: " << alignmentSequence[i] << " is a bad alignment point in sentence " << sentenceID << endl;
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return false;
    }
    // cout << "alignmentSequence[i] " << alignmentSequence[i] << " is " << s << ", " << t << endl;
    if ((size_t)t >= target.size() || (size_t)s >= source.size()) {
      cerr << "WARNING: sentence " << sentenceID << " has alignment point (" << s << ", " << t << ") out of bounds (" << source.size() << ", " << target.size() << ")\n";
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return false;
    }
    alignedToT[t].push_back( s );
    alignedToS[s].push_back( t );
    alignedCountS[s]++;
    alignedCountT[t]++;
  }
  return true;
}

