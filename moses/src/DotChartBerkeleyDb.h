#pragma once

#include <vector>
#include <cassert>
#include "ChartRule.h"
#include "WordConsumed.h"

namespace MosesBerkeleyPt
{
class SourcePhraseNode;
}


namespace Moses
{
class ProcessedRuleBerkeleyDb
{
	friend std::ostream& operator<<(std::ostream&, const ProcessedRuleBerkeleyDb&);

protected:
	const MosesBerkeleyPt::SourcePhraseNode &m_lastNode;
	const WordConsumed *m_wordsConsumed; // usually contains something, unless its the init processed rule
public:
	// used only to init dot stack.
	explicit ProcessedRuleBerkeleyDb(const MosesBerkeleyPt::SourcePhraseNode &lastNode)
		:m_lastNode(lastNode)
		,m_wordsConsumed(NULL)
	{}
	ProcessedRuleBerkeleyDb(const MosesBerkeleyPt::SourcePhraseNode &lastNode, const WordConsumed *wordsConsumed)
		:m_lastNode(lastNode)
		,m_wordsConsumed(wordsConsumed)
	{}
	~ProcessedRuleBerkeleyDb()
	{
		delete m_wordsConsumed;
	}
	const MosesBerkeleyPt::SourcePhraseNode &GetLastNode() const
	{ return m_lastNode; }
	const WordConsumed *GetLastWordConsumed() const
	{
		return m_wordsConsumed;
	}

	bool IsCurrNonTerminal() const
	{
		assert(m_wordsConsumed);
		return m_wordsConsumed->IsNonTerminal();
	}
	
/*
	inline int Compare(const ProcessedRule &compare) const
	{
		if (m_lastNode < compare.m_lastNode)
			return -1;
		if (m_lastNode > compare.m_lastNode)
			return 1;

		return m_wordsConsumed < compare.m_wordsConsumed;
	}
	inline bool operator<(const ProcessedRule &compare) const
	{
		return Compare(compare) < 0;
	}
*/
};

class ProcessedRuleCollBerkeleyDb
{
	friend std::ostream& operator<<(std::ostream&, const ProcessedRuleCollBerkeleyDb&);

protected:
	typedef std::vector<const ProcessedRuleBerkeleyDb*> CollType;
	CollType m_coll;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	const ProcessedRuleBerkeleyDb &Get(size_t ind) const
	{ return *m_coll[ind]; }

	void Add(const ProcessedRuleBerkeleyDb *processedRule)
	{
		m_coll.push_back(processedRule);
	}
	void Delete(size_t ind)
	{
		//delete m_coll[ind];
		m_coll.erase(m_coll.begin() + ind);
	}
	
	size_t GetSize() const
	{ return m_coll.size(); }
	
};

class SavedNodeBerkeleyDb
{
	const ProcessedRuleBerkeleyDb *m_processedRule;

public:
	SavedNodeBerkeleyDb(const ProcessedRuleBerkeleyDb *processedRule)
		:m_processedRule(processedRule)
	{
		assert(m_processedRule);
	}

	~SavedNodeBerkeleyDb()
	{
		delete m_processedRule;
	}

	const ProcessedRuleBerkeleyDb &GetProcessedRule() const
	{ return *m_processedRule; }
};

class ProcessedRuleStackBerkeleyDb
{ // coll of coll of processed rules
public:
	typedef std::vector<SavedNodeBerkeleyDb*> SavedNodeColl;

protected:
	typedef std::vector<ProcessedRuleCollBerkeleyDb*> CollType;
	CollType m_coll;

	SavedNodeColl m_savedNode;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	ProcessedRuleStackBerkeleyDb(size_t size);
	~ProcessedRuleStackBerkeleyDb();

	const ProcessedRuleCollBerkeleyDb &Get(size_t pos) const
	{ return *m_coll[pos]; }
	ProcessedRuleCollBerkeleyDb &Get(size_t pos)
	{ return *m_coll[pos]; }

	const ProcessedRuleCollBerkeleyDb &back() const
	{ return *m_coll.back(); }

	void Add(size_t pos, const ProcessedRuleBerkeleyDb *processedRule)
	{
		assert(processedRule);

		m_coll[pos]->Add(processedRule);
		m_savedNode.push_back(new SavedNodeBerkeleyDb(processedRule));
	}

	const SavedNodeColl &GetSavedNodeColl() const
	{ return m_savedNode; }

};

}

