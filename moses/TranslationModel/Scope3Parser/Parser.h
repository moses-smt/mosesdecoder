/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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

#include "moses/ChartRuleLookupManager.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/NonTerminal.h"
#include "moses/TranslationModel/RuleTable/UTrieNode.h"
#include "moses/TranslationModel/RuleTable/UTrie.h"
#include "moses/StaticData.h"
#include "ApplicableRuleTrie.h"
#include "StackLattice.h"
#include "StackLatticeBuilder.h"
#include "StackLatticeSearcher.h"
#include "VarSpanTrieBuilder.h"

#include <memory>
#include <vector>

namespace Moses
{

class InputType;
class ChartCellCollectionBase;
class ChartHypothesisCollection;
class Range;

/** @todo what is this?
 */
class Scope3Parser : public ChartRuleLookupManager
{
public:
  Scope3Parser(const ChartParser &parser,
               const ChartCellCollectionBase &cellColl,
               const RuleTableUTrie &ruleTable,
               size_t maxChartSpan)
    : ChartRuleLookupManager(parser, cellColl)
    , m_ruleTable(ruleTable)
    , m_maxChartSpan(maxChartSpan) {
    Init();
  }

  void GetChartRuleCollection(
    const InputPath &inputPath,
    size_t last,
    ChartParserCallback &outColl);

private:
  // Define a callback type for use by StackLatticeSearcher.
  struct MatchCallback {
  public:
    MatchCallback(const Range &range, ChartParserCallback &out)
      : m_range(range) , m_out(out) // , m_tpc(NULL)
    { }

    void operator()(const StackVec &stackVec) {
      m_out.Add(*m_tpc, stackVec, m_range);
    }
    const Range &m_range;
    ChartParserCallback &m_out;
    TargetPhraseCollection::shared_ptr m_tpc;
  };

  void Init();
  void InitRuleApplicationVector();
  void FillSentenceMap(SentenceMap &);
  void AddRulesToCells(const ApplicableRuleTrie &, std::pair<int, int>, int,
                       int);

  const RuleTableUTrie &m_ruleTable;
  std::vector<std::vector<std::vector<
  std::pair<const UTrieNode *, const VarSpanNode *> > > > m_ruleApplications;
  std::auto_ptr<VarSpanNode> m_varSpanTrie;
  StackVec m_emptyStackVec;
  const size_t m_maxChartSpan;
  StackLattice m_lattice;
  StackLatticeBuilder m_latticeBuilder;
  std::vector<VarSpanNode::NonTermRange> m_ranges;
  std::vector<std::vector<bool> > m_quickCheckTable;
};

}  // namespace Moses
