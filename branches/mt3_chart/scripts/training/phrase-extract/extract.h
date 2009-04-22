#pragma once

#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

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
		if ( otherHole.GetEnd() < GetStart() || otherHole.GetStart() > GetEnd()) return false;
		
		return true;
	}

};

typedef std::list<Hole> HoleList;

class HoleCollection
{
protected:
	HoleList m_sourceHoles, m_targetHoles;
	std::vector<Hole*> m_sortedTargetHoles;

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
	std::vector<Hole*> &GetSortedTargetHoles()
	{ return m_sortedTargetHoles; }

	void Add(int startE, int endE, int startF, int endF)
	{
		m_sourceHoles.push_back(Hole(startF, endF));
		m_targetHoles.push_back(Hole(startE, endE));
	}

	bool OverlapTarget(const Hole &targetHole) const
	{
		HoleList::const_iterator iter;
		for (iter = m_targetHoles.begin(); iter != m_targetHoles.end(); ++iter)
		{
			const Hole &currHole = *iter;
			bool overlap = currHole.Overlap(targetHole);
			if (overlap)
				return true;
		}
		return false;
	}

	void SortTargetHoles();
	
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

void HoleCollection::SortTargetHoles()
{
	assert(m_sortedTargetHoles.size() == 0);

	// add
	HoleList::iterator iter;
	for (iter = m_targetHoles.begin(); iter != m_targetHoles.end(); ++iter)
	{
		Hole &currHole = *iter;
		m_sortedTargetHoles.push_back(&currHole);
	}

	// sort
	std::sort(m_sortedTargetHoles.begin(), m_sortedTargetHoles.end(), HoleOrderer());
}

class PhraseExist
{
protected:
	std::vector< std::vector<HoleList> > m_phraseExist;
		// indexed by source pos. <int, int> are target pos

public:
	PhraseExist(size_t size)
		:m_phraseExist(size)
	{
		for (size_t pos = 0; pos < size; ++pos)
		{
			std::vector<HoleList> &endVec = m_phraseExist[pos];
			endVec.resize(size - pos);
		}
	}

	void Add(int startE, int endE, int startF, int endF)
	{
		m_phraseExist[startF][endF - startF].push_back(Hole(startE, endE));
	}
	const HoleList &GetTargetHoles(int startF, int endF) const
	{
		const HoleList &targetHoles = m_phraseExist[startF][endF - startF];
		return targetHoles;
	}

};

class SentenceAlignment {
 public:
  std::vector<std::string> english;
  std::vector<std::string> foreign;
  std::vector<int> alignedCountF;
  std::vector< std::vector<int> > alignedToE;

  int create( char[], char[], char[], int );
  //  void clear() { delete(alignment); };
};

// functions
void openFiles();
void extractBase( SentenceAlignment & );
void extract( SentenceAlignment & );
void addPhrase( SentenceAlignment &, int, int, int, int 
							, PhraseExist &phraseExist);
void addHieroPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF 
							 , PhraseExist &phraseExist, const HoleCollection &holeColl, int numHoles, int initStartF);

std::vector<std::string> tokenize( char [] );
bool isAligned ( SentenceAlignment &, int, int );

std::ofstream extractFile;
std::ofstream extractFileInv;
std::ofstream extractFileOrientation;
int maxPhraseLength;
int phraseCount = 0;
char* fileNameExtract;
bool orientationFlag = false;
bool onlyOutputSpanInfo = false;
bool noFileLimit = false;
bool zipFiles = false;
bool properConditioning = false;
int maxNonTerm = 0;
bool nonTermFirstWord = false;
bool nonTermConsec = false;

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
