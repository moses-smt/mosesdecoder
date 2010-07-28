/*
 *  PhraseAlignment.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 28/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "PhraseAlignment.h"
#include "SafeGetline.h"
#include "tables-core.h"
#include "score.h"

using namespace std;

extern Vocabulary vcbT;
extern Vocabulary vcbS;

extern PhraseTable phraseTableT;
extern PhraseTable phraseTableS;
extern bool hierarchicalFlag;

void PhraseAlignment::addToCount( char line[] ) 
{
	vector< string > token = tokenize( line );
	int item = 0;
	for (int j=0; j<token.size(); j++) 
	{
		if (token[j] == "|||") item++;
		if (item == 4)
		{
			float addCount;
			sscanf(token[j].c_str(), "%f", &addCount);
			count += addCount;
		}
	}
	if (item < 4) // no specified counts -> counts as one
		count += 1.0;
}

// read in a phrase pair and store it
void PhraseAlignment::create( char line[], int lineID ) 
{
	//cerr << "processing " << line;
	vector< string > token = tokenize( line );
	int item = 1;
	PHRASE phraseS, phraseT;
	for (int j=0; j<token.size(); j++) 
	{
		if (token[j] == "|||") item++;
		else if (item == 1) // source phrase
		{
			phraseS.push_back( vcbS.storeIfNew( token[j] ) );
		}
		
		else if (item == 2) // target phrase
		{
			phraseT.push_back( vcbT.storeIfNew( token[j] ) );
		}
		
		else if (item == 3) // alignment
		{
			int s,t;
			sscanf(token[j].c_str(), "%d-%d", &s, &t);
			if (t >= phraseT.size() || s >= phraseS.size()) 
			{ 
				cerr << "WARNING: phrase pair " << lineID 
				<< " has alignment point (" << s << ", " << t 
				<< ") out of bounds (" << phraseS.size() << ", " << phraseT.size() << ")\n";
			}
			else 
			{
				// first alignment point? -> initialize
				if (alignedToT.size() == 0) 
				{
          assert(alignedToS.size() == 0);
          size_t numTgtSymbols = (hierarchicalFlag ? phraseT.size()-1 : phraseT.size());
          alignedToT.resize(numTgtSymbols);
          size_t numSrcSymbols = (hierarchicalFlag ? phraseS.size()-1 : phraseS.size());
          alignedToS.resize(numSrcSymbols);
					source = phraseTableS.storeIfNew( phraseS );
					target = phraseTableT.storeIfNew( phraseT );
				}
				// add alignment point
				alignedToT[t].insert( s );
				alignedToS[s].insert( t );
			}
		}
		else if (item == 4) // count
		{
			sscanf(token[j].c_str(), "%f", &count);
		}
	}
	if (item == 3)
	{
		count = 1.0;
	}
	if (item < 3 || item > 4)
	{
		cerr << "ERROR: faulty line " << lineID << ": " << line << endl;
	}
}

void PhraseAlignment::clear() {
  alignedToT.clear();
  alignedToS.clear();
}

// check if two word alignments between a phrase pair are the same
bool PhraseAlignment::equals( const PhraseAlignment& other ) 
{
	if (this == &other) return true;
	if (other.target != target) return false;
	if (other.source != source) return false;
	if (other.alignedToT != alignedToT) return false;
	if (other.alignedToS != alignedToS) return false;
	return true;
}

// check if two word alignments between a phrase pairs "match"
// i.e. they do not differ in the alignment of non-termimals
bool PhraseAlignment::match( const PhraseAlignment& other )
{
	if (this == &other) return true;
	if (other.target != target) return false;
	if (other.source != source) return false;
	if (!hierarchicalFlag) return true;
	
	PHRASE phraseT = phraseTableT.getPhrase( target );
	
  assert(phraseT.size() == alignedToT.size() + 1);
  assert(alignedToT.size() == other.alignedToT.size());
	
	// loop over all words (note: 0 = left hand side of rule)
	for(int i=0;i<phraseT.size()-1;i++)
	{
		if (isNonTerminal( vcbT.getWord( phraseT[i] ) ))
		{
			if (alignedToT[i].size() != 1 ||
			    other.alignedToT[i].size() != 1 ||
					*(alignedToT[i].begin()) != *(other.alignedToT[i].begin()))
				return false;
		}
	}
	return true;
}


