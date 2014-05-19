/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2011 University of Edinburgh

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

#include "ChartRuleLookupManagerCYKPlus.h"
#include "CompletedRuleCollection.h"
#include "moses/NonTerminal.h"
#include "moses/TranslationModel/CompactPT/PhraseDictionaryCompact.h"
#include "moses/StackVec.h"

namespace Moses
{

class ChartParserCallback;
class WordsRange;

class TargetPhraseCollections
{
  std::vector<TargetPhraseCollections*> m_coll;
};

class CompactNode
{
public:

  std::string m_phrase;
  size_t m_endPos;
};

class CompactNodes
{
public:
	size_t GetSize() const
	{ return m_coll.size(); }

	const CompactNode &Get(size_t i) const
	{ return *m_coll[i]; }

	std::vector<CompactNode*> m_coll;

};

//! Implementation of ChartRuleLookupManager for in-memory rule tables.
class ChartRuleLookupManagerCompactPT : public ChartRuleLookupManagerCYKPlus
{
public:
  ChartRuleLookupManagerCompactPT(const ChartParser &parser,
                               const ChartCellCollectionBase &cellColl,
                               const PhraseDictionaryCompact &ruleTable);

  ~ChartRuleLookupManagerCompactPT() {};

  virtual void GetChartRuleCollection(
    const WordsRange &range,
    size_t lastPos, // last position to consider if using lookahead
    ChartParserCallback &outColl);

private:
  const PhraseDictionaryCompact &m_ruleTable;

  // temporary storage of completed rules (one collection per end position; all rules collected consecutively start from the same position)
  std::vector<TargetPhraseCollections> m_completedRules;
  std::vector<CompactNodes> m_activeChart;

  void ExtendTerm(size_t pos);
  void ExtendNonTerm(size_t startPos, size_t endPos);

};

}  // namespace Moses

