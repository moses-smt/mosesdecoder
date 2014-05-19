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
  std::vector<CompletedRuleCollection> m_completedRules;

  size_t m_lastPos;
  size_t m_unaryPos;

  StackVec m_stackVec;
  ChartParserCallback* m_outColl;

};

}  // namespace Moses

