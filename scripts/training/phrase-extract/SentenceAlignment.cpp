/*
 *  SentenceAlignment.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <set>
#include <map>
#include "SentenceAlignment.h"
#include "XmlTree.h"
#include "tables-core.h"

using namespace std;

extern std::set< std::string > targetLabelCollection, sourceLabelCollection;
extern std::map< std::string, int > targetTopLabelCollection, sourceTopLabelCollection;

int SentenceAlignment::create( char targetString[], char sourceString[], char alignmentString[], int sentenceID, const Global &global ) {
  // tokenizing English (and potentially extract syntax spans)
  if (global.targetSyntax) {
		string targetStringCPP = string(targetString);
		ProcessAndStripXMLTags( targetStringCPP, targetTree, targetLabelCollection , targetTopLabelCollection );
		target = tokenize( targetStringCPP.c_str() );
		// cerr << "E: " << targetStringCPP << endl;
  }
  else {
		target = tokenize( targetString );
  }
	
  // tokenizing source (and potentially extract syntax spans)
  if (global.sourceSyntax) {
		string sourceStringCPP = string(sourceString);
		ProcessAndStripXMLTags( sourceStringCPP, sourceTree, sourceLabelCollection , sourceTopLabelCollection );
		source = tokenize( sourceStringCPP.c_str() );
		// cerr << "F: " << sourceStringCPP << endl;
  }
  else {
		source = tokenize( sourceString );
  }
	
  // check if sentences are empty
  if (target.size() == 0 || source.size() == 0) {
    cerr << "no target (" << target.size() << ") or source (" << source.size() << ") words << end insentence " << sentenceID << endl;
    cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
    return 0;
  }
	
  // prepare data structures for alignments
  for(int i=0; i<source.size(); i++) {
    alignedCountS.push_back( 0 );
  }
  for(int i=0; i<target.size(); i++) {
    vector< int > dummy;
    alignedToT.push_back( dummy );
  }
	
  // reading in alignments
  vector<string> alignmentSequence = tokenize( alignmentString );
  for(int i=0; i<alignmentSequence.size(); i++) {
    int s,t;
    // cout << "scaning " << alignmentSequence[i].c_str() << endl;
    if (! sscanf(alignmentSequence[i].c_str(), "%d-%d", &s, &t)) {
      cerr << "WARNING: " << alignmentSequence[i] << " is a bad alignment point in sentence " << sentenceID << endl; 
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return 0;
    }
		// cout << "alignmentSequence[i] " << alignmentSequence[i] << " is " << s << ", " << t << endl;
    if (t >= target.size() || s >= source.size()) { 
      cerr << "WARNING: sentence " << sentenceID << " has alignment point (" << s << ", " << t << ") out of bounds (" << source.size() << ", " << target.size() << ")\n";
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return 0;
    }
    alignedToT[t].push_back( s );
    alignedCountS[s]++;
  }
  return 1;
}
