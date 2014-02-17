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
#include <sstream>
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
:m_tunnelCollection(NULL)
,m_lattice(NULL)
{}

SentenceAlignment::~SentenceAlignment()
{
	delete m_tunnelCollection;
	delete m_lattice;
}

int SentenceAlignment::Create( const std::string &targetString, const std::string &sourceString, const std::string &alignmentString, int sentenceID, const Global &global )
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
	
	//InitTightest(m_s2tTightest, source.size());
	//InitTightest(m_t2sTightest, target.size());

	
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
		
		//SetAlignment(s, t);
  }
	
	bool mixed = global.mixed;
	sourceTree.AddDefaultNonTerms(global.sourceSyntax, mixed, source.size());
	targetTree.AddDefaultNonTerms(global.targetSyntax, mixed, target.size());

	//CalcTightestSpan(m_s2tTightest);
	//CalcTightestSpan(m_t2sTightest);
	
  return 1;
}

/*
void SentenceAlignment::InitTightest(Outer &tightest, size_t len)
{
	tightest.resize(len);
	
	for (size_t posOuter = 0; posOuter < len; ++posOuter)
	{
		Inner &inner = tightest[posOuter];
		size_t innerSize = len - posOuter;
		inner.resize(innerSize);
		
	}
}

void SentenceAlignment::CalcTightestSpan(Outer &tightest)
{
	size_t len = tightest.size();
	
	for (size_t startPos = 0; startPos < len; ++startPos)
	{
		for (size_t endPos = startPos + 1; endPos < len; ++endPos)
		{
			const Range &prevRange = GetTightest(tightest, startPos, endPos - 1);
			const Range &smallRange = GetTightest(tightest, endPos, endPos); 
			Range &newRange = GetTightest(tightest, startPos, endPos);
			
			newRange.Merge(prevRange, smallRange);
			//cerr << "[" << startPos << "-" << endPos << "] --> [" << newRange.GetStartPos() << "-" << newRange.GetEndPos() << "]";
		}
	}
}

Range &SentenceAlignment::GetTightest(Outer &tightest, size_t startPos, size_t endPos)
{
	assert(endPos < tightest.size());
	assert(endPos >= startPos);
	
	Inner &inner = tightest[startPos];
	
	size_t ind = endPos - startPos;
	Range &ret = inner[ind];
	return ret;
}

void SentenceAlignment::SetAlignment(size_t source, size_t target)
{
	SetAlignment(m_s2tTightest, source, target);
	SetAlignment(m_t2sTightest, target, source);
}

void SentenceAlignment::SetAlignment(Outer &tightest, size_t thisPos, size_t thatPos)
{

	Range &range = GetTightest(tightest, thisPos, thisPos);
	if (range.GetStartPos() == NOT_FOUND)
	{ // not yet set, do them both
		assert(range.GetEndPos() == NOT_FOUND);
		range.SetStartPos(thatPos);
		range.SetEndPos(thatPos);
	}
	else
	{
		assert(range.GetEndPos() != NOT_FOUND);
		range.SetStartPos( (range.GetStartPos() > thatPos) ? thatPos : range.GetStartPos() );
		range.SetEndPos( (range.GetEndPos() < thatPos) ? thatPos : range.GetEndPos() );
	}
}
 */


void SentenceAlignment::FindTunnels(const Global &global ) 
{
	int countT = target.size();
	int countS = source.size();
	int maxSpan = max(global.maxHoleSpanSourceDefault, global.maxHoleSpanSourceSyntax);

	m_tunnelCollection = new TunnelCollection(countS);
	
	m_tunnelCollection->alignedCountS = alignedCountS;
	m_tunnelCollection->alignedCountT.resize(alignedToT.size());
	for (size_t ind = 0; ind < alignedToT.size(); ind++)
	{
		m_tunnelCollection->alignedCountT[ind] = alignedToT[ind].size();
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
			
			if (m_tunnelCollection->NumUnalignedWord(1, startT, endT) >= global.maxUnaligned)
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
					if (m_tunnelCollection->NumUnalignedWord(0, startS, endS) >= global.maxUnaligned)
						continue;
					
					// take note that this is a valid phrase alignment
					m_tunnelCollection->Add(startS, endS, startT, endT);
				}
			}
		}
	}
	
	//cerr << *tunnelCollection << endl;

}

void SentenceAlignment::CreateLattice(const Global &global)
{
	size_t countS = source.size();
	m_lattice = new Lattice(countS);
	
	for (size_t startPos = 0; startPos < countS; ++startPos)
	{
		//cerr << "creating arcs for " << startPos << "=";
		m_lattice->CreateArcs(startPos, *m_tunnelCollection, *this, global);
		
		//cerr << LatticeNode::s_count << endl;
	}
}

void SentenceAlignment::CreateRules(const Global &global)
{
	size_t countS = source.size();
	
	for (size_t startPos = 0; startPos < countS; ++startPos)
	{
		//cerr << "creating rules for " << startPos << "\n";
		m_lattice->CreateRules(startPos, *this, global);
	}
}

void OutputSentenceStr(std::ostream &out, const std::vector<std::string> &vec)
{
	for (size_t pos = 0; pos < vec.size(); ++pos)
	{
		out << vec[pos] << " ";
	}
}

std::ostream& operator<<(std::ostream &out, const SentenceAlignment &obj)
{	
	OutputSentenceStr(out, obj.target);
	out << " ==> ";
	OutputSentenceStr(out, obj.source);
	out << endl;
	
	out << *obj.m_tunnelCollection;	

	if (obj.m_lattice)
		out << endl << *obj.m_lattice;
	
	return out;
}




