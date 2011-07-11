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
#include "TunnelCollection.h"
#include "Lattice.h"
#include "LatticeNode.h"

using namespace std;

extern std::set< std::string > targetLabelCollection, sourceLabelCollection;
extern std::map< std::string, int > targetTopLabelCollection, sourceTopLabelCollection;

SentenceAlignment::SentenceAlignment()
:holeCollection(NULL)
,lattice(NULL)
{}

SentenceAlignment::~SentenceAlignment()
{
	delete holeCollection;
	delete lattice;
}

int SentenceAlignment::create( const std::string &targetString, const std::string &sourceString, const std::string &alignmentString, int sentenceID, const Global &global )
{

  // tokenizing English (and potentially extract syntax spans)
  if (global.targetSyntax) {
		string targetStringCPP = string(targetString);
		ProcessAndStripXMLTags( targetStringCPP, targetTree, targetLabelCollection , targetTopLabelCollection );
		target = tokenize( targetStringCPP.c_str() );
		// cerr << "E: " << targetStringCPP << endl;
  }
  else {
		target = tokenize( targetString.c_str() );
  }
	
  // tokenizing source (and potentially extract syntax spans)
  if (global.sourceSyntax) {
		string sourceStringCPP = string(sourceString);
		ProcessAndStripXMLTags( sourceStringCPP, sourceTree, sourceLabelCollection , sourceTopLabelCollection );
		source = tokenize( sourceStringCPP.c_str() );
		// cerr << "F: " << sourceStringCPP << endl;
  }
  else {
		source = tokenize( sourceString.c_str() );
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
  vector<string> alignmentSequence = tokenize( alignmentString.c_str() );
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
	
	bool mixed = global.mixed;
	sourceTree.AddDefaultNonTerms(global.sourceSyntax, mixed, source.size());
	targetTree.AddDefaultNonTerms(global.targetSyntax, mixed, target.size());

  return 1;
}

void SentenceAlignment::FindAlignedHoles(const Global &global ) 
{
	int countT = target.size();
	int countS = source.size();
	int maxSpan = max(global.maxHoleSpanSourceDefault, global.maxHoleSpanSourceSyntax);

	holeCollection = new TunnelCollection(countS);
	
	holeCollection->alignedCountS = alignedCountS;
	holeCollection->alignedCountT.resize(alignedToT.size());
	for (size_t ind = 0; ind < alignedToT.size(); ind++)
	{
		holeCollection->alignedCountT[ind] = alignedToT[ind].size();
	}
	
	// phrase repository for creating hiero phrases
	
	// check alignments for target phrase startT...endT
	for(int lengthT=1;
			lengthT <= maxSpan && lengthT <= countT;
			lengthT++) {
		for(int startT=0; startT < countT-(lengthT-1); startT++) {
			
			// that's nice to have
			int endT = startT + lengthT - 1;
			
			// if there is target side syntax, there has to be a node
			if (global.targetSyntax && !targetTree.HasNode(startT,endT))
				continue;
			
			// find find aligned source words
			// first: find minimum and maximum source word
			int minS = 9999;
			int maxS = -1;
			vector< int > usedS = alignedCountS;
			for(int ti=startT;ti<=endT;ti++) {
				for(int i=0;i<alignedToT[ti].size();i++) {
					int si = alignedToT[ti][i];
					// cerr << "point (" << si << ", " << ti << ")\n";
					if (si<minS) { minS = si; }
					if (si>maxS) { maxS = si; }
					usedS[ si ]--;
				}
			}
			
			// unaligned phrases are not allowed
			if( maxS == -1 )
				continue;
			
			// source phrase has to be within limits
			if( maxS-minS >= maxSpan )
			{
				continue;
			}
			
			// check if source words are aligned to out of bound target words
			bool out_of_bounds = false;
			for(int si=minS;si<=maxS && !out_of_bounds;si++)
			{
				if (usedS[si]>0) {
					out_of_bounds = true;
				}
			}
			
			// if out of bound, you gotta go
			if (out_of_bounds)
				continue;
			
			if (holeCollection->NumUnalignedWord(1, startT, endT) >= global.maxUnaligned)
				continue;
			
			// done with all the checks, lets go over all consistent phrase pairs
			// start point of source phrase may retreat over unaligned
			for(int startS=minS;
					(startS>=0 &&
					 startS>maxS - maxSpan && // within length limit
					 (startS==minS || alignedCountS[startS]==0)); // unaligned
					startS--)
			{
				// end point of source phrase may advance over unaligned
				for(int endS=maxS;
						(endS<countS && endS<startS + maxSpan && // within length limit
						 (endS==maxS || alignedCountS[endS]==0)); // unaligned
						endS++) 
				{
					if (holeCollection->NumUnalignedWord(0, startS, endS) >= global.maxUnaligned)
						continue;
					
					// take note that this is a valid phrase alignment
					holeCollection->Add(startS, endS, startT, endT);
				}
			}
		}
	}
	
	//cerr << *holeCollection << endl;

}

void SentenceAlignment::CreateLattice(const Global &global)
{
	size_t countS = source.size();
	lattice = new Lattice(countS);
	
	for (size_t startPos = 0; startPos < countS; ++startPos)
	{
		//cerr << "creating arcs for " << startPos << "=";
		lattice->CreateArcs(startPos, *holeCollection, *this, global);
		
		//cerr << LatticeNode::s_count << endl;
	}
}

void SentenceAlignment::CreateRules(const Global &global)
{
	size_t countS = source.size();
	
	for (size_t startPos = 0; startPos < countS; ++startPos)
	{
		//cerr << "creating rules for " << startPos << "\n";
		lattice->CreateRules(startPos, *this, global);
	}
}



