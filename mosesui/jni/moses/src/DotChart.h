// $Id: DotChart.h 3561 2010-09-23 17:39:32Z hieuhoang1972 $
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
#include "PhraseDictionaryNodeSCFG.h"
#include "ChartTranslationOption.h"
#include "WordConsumed.h"

namespace Moses
{

class ProcessedRule
{
	friend std::ostream& operator<<(std::ostream&, const ProcessedRule&);

protected:
	const PhraseDictionaryNodeSCFG &m_lastNode;
	const WordConsumed *m_wordsConsumed; // usually contains something, unless its the init processed rule
public:
	// used only to init dot stack.
	explicit ProcessedRule(const PhraseDictionaryNodeSCFG &lastNode)
		:m_lastNode(lastNode)
		,m_wordsConsumed(NULL)
	{}
	ProcessedRule(const PhraseDictionaryNodeSCFG &lastNode, const WordConsumed *wordsConsumed)
		:m_lastNode(lastNode)
		,m_wordsConsumed(wordsConsumed)
	{}
	~ProcessedRule()
	{
		delete m_wordsConsumed;
	}
	const PhraseDictionaryNodeSCFG &GetLastNode() const
	{ return m_lastNode; }
	const WordConsumed *GetLastWordConsumed() const
	{
		return m_wordsConsumed;
	}
};

typedef std::vector<const ProcessedRule*> ProcessedRuleList;

// Collection of all ProcessedRules that share a common start point,
// grouped by end point.  Additionally, maintains a list of all
// ProcessedRules that could be expanded further, i.e. for which the
// corresponding PhraseDictionaryNodeSCFG is not a leaf.
class ProcessedRuleColl
{
protected:
	typedef std::vector<ProcessedRuleList> CollType;
	CollType m_coll;
    ProcessedRuleList m_runningNodes;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	ProcessedRuleColl(size_t size)
      : m_coll(size)
    {}

	~ProcessedRuleColl();

	const ProcessedRuleList &Get(size_t pos) const
	{ return m_coll[pos]; }
	ProcessedRuleList &Get(size_t pos)
	{ return m_coll[pos]; }

	void Add(size_t pos, const ProcessedRule *processedRule)
	{
		assert(processedRule);
		m_coll[pos].push_back(processedRule);
        if (!processedRule->GetLastNode().IsLeaf())
        {
		    m_runningNodes.push_back(processedRule);
        }
	}

	const ProcessedRuleList &GetRunningNodes() const
	{ return m_runningNodes; }

};

}

