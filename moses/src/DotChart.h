// $Id: DotChart.h 3048 2010-04-05 17:25:26Z hieuhoang1972 $
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
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
#pragma once

#include <vector>
#include <cassert>
#include "PhraseDictionaryNode.h"
#include "ChartRule.h"
#include "WordConsumed.h"

namespace Moses
{

class ProcessedRule
{
	friend std::ostream& operator<<(std::ostream&, const ProcessedRule&);

protected:
	const PhraseDictionaryNode &m_lastNode;
	const WordConsumed *m_wordsConsumed; // usually contains something, unless its the init processed rule
public:
	// used only to init dot stack.
	explicit ProcessedRule(const PhraseDictionaryNode &lastNode)
		:m_lastNode(lastNode)
		,m_wordsConsumed(NULL)
	{}
	ProcessedRule(const PhraseDictionaryNode &lastNode, const WordConsumed *wordsConsumed)
		:m_lastNode(lastNode)
		,m_wordsConsumed(wordsConsumed)
	{}
	~ProcessedRule()
	{
		delete m_wordsConsumed;
	}
	const PhraseDictionaryNode &GetLastNode() const
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

class ProcessedRuleColl
{
	friend std::ostream& operator<<(std::ostream&, const ProcessedRuleColl&);

protected:
	typedef std::vector<const ProcessedRule*> CollType;
	CollType m_coll;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	const ProcessedRule &Get(size_t ind) const
	{ return *m_coll[ind]; }

	void Add(const ProcessedRule *processedRule)
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

class SavedNode
{
	const ProcessedRule *m_processedRule;

public:
	SavedNode(const ProcessedRule *processedRule)
		:m_processedRule(processedRule)
	{
		assert(m_processedRule);
	}

	~SavedNode()
	{
		delete m_processedRule;
	}

	const ProcessedRule &GetProcessedRule() const
	{ return *m_processedRule; }
};

class ProcessedRuleStack
{ // coll of coll of processed rules
public:
	typedef std::vector<SavedNode*> SavedNodeColl;

protected:
	typedef std::vector<ProcessedRuleColl*> CollType;
	CollType m_coll;

	SavedNodeColl m_savedNode;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	ProcessedRuleStack(size_t size);
	~ProcessedRuleStack();

	const ProcessedRuleColl &Get(size_t pos) const
	{ return *m_coll[pos]; }
	ProcessedRuleColl &Get(size_t pos)
	{ return *m_coll[pos]; }

	const ProcessedRuleColl &back() const
	{ return *m_coll.back(); }

	void Add(size_t pos, const ProcessedRule *processedRule)
	{
		assert(processedRule);

		m_coll[pos]->Add(processedRule);
		m_savedNode.push_back(new SavedNode(processedRule));
	}

	const SavedNodeColl &GetSavedNodeColl() const
	{ return m_savedNode; }

};

}

