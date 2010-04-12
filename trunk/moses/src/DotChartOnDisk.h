// $Id: DotChartOnDisk.h 3048 2010-04-05 17:25:26Z hieuhoang1972 $
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
#include "ChartRule.h"
#include "WordConsumed.h"

namespace OnDiskPt
{
class PhraseNode;
}


namespace Moses
{
class ProcessedRuleOnDisk
{
	friend std::ostream& operator<<(std::ostream&, const ProcessedRuleOnDisk&);

protected:
	const OnDiskPt::PhraseNode &m_lastNode;
	const WordConsumed *m_wordsConsumed; // usually contains something, unless its the init processed rule
	mutable bool m_done;
public:
	// used only to init dot stack.
	explicit ProcessedRuleOnDisk(const OnDiskPt::PhraseNode &lastNode)
		:m_lastNode(lastNode)
		,m_wordsConsumed(NULL)
		,m_done(false)
	{}
	ProcessedRuleOnDisk(const OnDiskPt::PhraseNode &lastNode, const WordConsumed *wordsConsumed)
		:m_lastNode(lastNode)
		,m_wordsConsumed(wordsConsumed)
		,m_done(false)
	{}
	~ProcessedRuleOnDisk()
	{
		delete m_wordsConsumed;
	}
	const OnDiskPt::PhraseNode &GetLastNode() const
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
	
	bool Done() const
	{ return m_done; }
	void Done(bool value) const
	{ m_done = value; }

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

class ProcessedRuleCollOnDisk
{
	friend std::ostream& operator<<(std::ostream&, const ProcessedRuleCollOnDisk&);

protected:
	typedef std::vector<const ProcessedRuleOnDisk*> CollType;
	CollType m_coll;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	const ProcessedRuleOnDisk &Get(size_t ind) const
	{ return *m_coll[ind]; }

	void Add(const ProcessedRuleOnDisk *processedRule)
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

class SavedNodeOnDisk
{
	const ProcessedRuleOnDisk *m_processedRule;

public:
	SavedNodeOnDisk(const ProcessedRuleOnDisk *processedRule)
		:m_processedRule(processedRule)
	{
		assert(m_processedRule);
	}

	~SavedNodeOnDisk()
	{
		delete m_processedRule;
	}

	const ProcessedRuleOnDisk &GetProcessedRule() const
	{ return *m_processedRule; }
};

class ProcessedRuleStackOnDisk
{ // coll of coll of processed rules
public:
	typedef std::vector<SavedNodeOnDisk*> SavedNodeColl;

protected:
	typedef std::vector<ProcessedRuleCollOnDisk*> CollType;
	CollType m_coll;

	SavedNodeColl m_savedNode;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	ProcessedRuleStackOnDisk(size_t size);
	~ProcessedRuleStackOnDisk();

	const ProcessedRuleCollOnDisk &Get(size_t pos) const
	{ return *m_coll[pos]; }
	ProcessedRuleCollOnDisk &Get(size_t pos)
	{ return *m_coll[pos]; }

	const ProcessedRuleCollOnDisk &back() const
	{ return *m_coll.back(); }

	void Add(size_t pos, const ProcessedRuleOnDisk *processedRule)
	{
		assert(processedRule);

		m_coll[pos]->Add(processedRule);
		m_savedNode.push_back(new SavedNodeOnDisk(processedRule));
	}

	const SavedNodeColl &GetSavedNodeColl() const
	{ return m_savedNode; }
	
	void SortSavedNodes();

};

}

