#pragma once

#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "SyntaxTree.h"
#include "XmlTree.h"

class Hole
{
protected:
	int m_start, m_end, m_pos;

public:
	Hole(const Hole &copy)
		:m_start(copy.m_start)
		,m_end(copy.m_end)
	{}
	Hole(int startPos, int endPos)
		:m_start(startPos)
		,m_end(endPos)
	{}

	int GetStart() const
	{ return m_start; }
	int GetEnd() const
	{ return m_end; }
	
	void SetPos(int pos)
	{ m_pos = pos; }
	int GetPos() const
	{ return m_pos; }
	
	bool Overlap(const Hole &otherHole) const
	{
		return ! ( otherHole.GetEnd()   < GetStart() || 
							 otherHole.GetStart() > GetEnd() );
	}

	bool Neighbor(const Hole &otherHole) const
	{
		return ( otherHole.GetEnd()+1 == GetStart() || 
						 otherHole.GetStart() == GetEnd()+1 ); 
	}


};

typedef std::list<Hole> HoleList;
typedef std::map< int, int > WordIndex;

class HoleCollection
{
protected:
	HoleList m_sourceHoles, m_targetHoles;
	std::vector<Hole*> m_sortedSourceHoles;

public:
	HoleCollection()
	{}
	HoleCollection(const HoleCollection &copy)
		:m_sourceHoles(copy.m_sourceHoles)
		,m_targetHoles(copy.m_targetHoles)
	{} // don't copy sorted target holes. messes up sorting fn

	const HoleList &GetSourceHoles() const
	{ return m_sourceHoles; }
	const HoleList &GetTargetHoles() const
	{ return m_targetHoles; }

	HoleList &GetSourceHoles()
	{ return m_sourceHoles; }
	HoleList &GetTargetHoles()
	{ return m_targetHoles; }
	std::vector<Hole*> &GetSortedSourceHoles()
	{ return m_sortedSourceHoles; }

	void Add(int startT, int endT, int startS, int endS)
	{
		m_sourceHoles.push_back(Hole(startS, endS));
		m_targetHoles.push_back(Hole(startT, endT));
	}

	bool OverlapSource(const Hole &sourceHole) const
	{
		HoleList::const_iterator iter;
		for (iter = m_sourceHoles.begin(); iter != m_sourceHoles.end(); ++iter)
		{
			const Hole &currHole = *iter;
			if (currHole.Overlap(sourceHole))
				return true;
		}
		return false;
	}
	
	bool ConsecSource(const Hole &sourceHole) const
	{
		HoleList::const_iterator iter;
		for (iter = m_sourceHoles.begin(); iter != m_sourceHoles.end(); ++iter)
		{
			const Hole &currHole = *iter;
			if (currHole.Neighbor(sourceHole))
				return true;
		}
		return false;
	}

	void SortSourceHoles();
	
};

class HoleOrderer
{
public:
	bool operator()(const Hole* holeA, const Hole* holeB) const
	{
		assert(holeA->GetStart() != holeB->GetStart());
		return holeA->GetStart() < holeB->GetStart();
	}
};

void HoleCollection::SortSourceHoles()
{
	assert(m_sortedSourceHoles.size() == 0);

	// add
	HoleList::iterator iter;
	for (iter = m_sourceHoles.begin(); iter != m_sourceHoles.end(); ++iter)
	{
		Hole &currHole = *iter;
		m_sortedSourceHoles.push_back(&currHole);
	}

	// sort
	std::sort(m_sortedSourceHoles.begin(), m_sortedSourceHoles.end(), HoleOrderer());
}

// reposity of extracted phrase pairs
// which are potential holes in larger phrase pairs
class RuleExist
{
protected:
	std::vector< std::vector<HoleList> > m_phraseExist;
		// indexed by source pos. and source length 
		// maps to list of holes where <int, int> are target pos

public:
	RuleExist(size_t size)
		:m_phraseExist(size)
	{
		// size is the length of the source sentence
		for (size_t pos = 0; pos < size; ++pos)
		{
			// create empty hole lists
			std::vector<HoleList> &endVec = m_phraseExist[pos];
			endVec.resize(size - pos);
		}
	}

	void Add(int startT, int endT, int startS, int endS)
	{
		// m_phraseExist[startS][endS - startS].push_back(Hole(startT, endT));
		m_phraseExist[startT][endT - startT].push_back(Hole(startS, endS));
	}
	//const HoleList &GetTargetHoles(int startS, int endS) const
	//{
	//	const HoleList &targetHoles = m_phraseExist[startS][endS - startS];
	//	return targetHoles;
	//}
	const HoleList &GetSourceHoles(int startT, int endT) const
	{
		const HoleList &sourceHoles = m_phraseExist[startT][endT - startT];
		return sourceHoles;
	}

};

class SentenceAlignment {
 public:
  std::vector<std::string> target;
  std::vector<std::string> source;
  std::vector<int> alignedCountS;
  std::vector< std::vector<int> > alignedToT;
  SyntaxTree targetTree;
  SyntaxTree sourceTree;

  int create( char[], char[], char[], int );
  //  void clear() { delete(alignment); };
};

// sentence-level collection of rules
class ExtractedRule {
public:
	std::string source,target,alignment,alignmentInv,orientation,orientationForward;
	int startT,endT,startS,endS;
	float count; 
ExtractedRule( int sT,int eT,int sS,int eS )
	:startT(sT),endT(eT),startS(sS),endS(eS) {
		source = "";
		target = "";
		alignment = "";
		alignmentInv = "";
		orientation = "";
		orientationForward = "";
		count = 0;
		// countInv = 0;
	}
};

std::vector< ExtractedRule > extractedRules;
void extractRules( SentenceAlignment & );
void addRuleToCollection( ExtractedRule &rule );
void consolidateRules();
void writeRulesToFile();
void writeGlueGrammar( std::string );

typedef std::vector< int > LabelIndex;

// functions
void openFiles();
void addRule( SentenceAlignment &, int, int, int, int 
							, RuleExist &ruleExist);
void addHieroRule( SentenceAlignment &sentence, int startT, int endT, int startS, int endS 
									 , RuleExist &ruleExist, const HoleCollection &holeColl, int numHoles, int initStartF, int wordCountT, int wordCountS);

std::vector<std::string> tokenize( const char [] );
bool isAligned ( SentenceAlignment &, int, int );

std::ofstream extractFile;
std::ofstream extractFileInv;
std::ofstream extractFileOrientation;
int maxSpan = 12;
int minHoleSource = 2;
int minHoleTarget = 1;
int minWords = 1;
int maxSymbolsTarget = 999;
int maxSymbolsSource = 5;
// int minHoleSize = 1;
// int minSubPhraseSize = 1; // minimum size of a remaining lexical phrase 
int phraseCount = 0;
bool onlyDirectFlag = false;
bool orientationFlag = false;
bool glueGrammarFlag = false;
std::set< std::string > targetLabelCollection, sourceLabelCollection;
std::map< std::string, int > targetTopLabelCollection, sourceTopLabelCollection;
bool hierarchicalFlag = false;
bool onlyOutputSpanInfo = false;
bool noFileLimit = false;
//bool zipFiles = false;
bool properConditioning = false;
int maxNonTerm = 2;
bool nonTermFirstWord = true;
bool nonTermConsecTarget = true;
bool nonTermConsecSource = false;
bool requireAlignedWord = true;
bool sourceSyntax = false;
bool targetSyntax = false;
bool duplicateRules = true;
bool fractionalCounting = true;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) { \
                _IS.getline(_LINE, _SIZE, _DELIM); \
                if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear(); \
                if (_IS.gcount() == _SIZE-1) { \
                  cerr << "Line too long! Buffer overflow. Delete lines >=" \
                    << _SIZE << " chars or raise LINE_MAX_LENGTH in phrase-extract/extract.cpp" \
                    << endl; \
                    exit(1); \
                } \
              }
#define LINE_MAX_LENGTH 60000

std::string IntToString( int i )
{
	std::string s;
	std::stringstream out;
	out << i;
	return out.str();
}

