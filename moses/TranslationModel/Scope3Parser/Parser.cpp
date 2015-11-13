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

#include "Parser.h"

#include "moses/ChartParser.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/InputType.h"
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

void Scope3Parser::GetChartRuleCollection(
  const InputPath &inputPath,
  size_t last,
  ChartParserCallback &outColl)
{
  const Range &range = inputPath.GetWordsRange();
  const size_t start = range.GetStartPos();
  const size_t end = range.GetEndPos();

  std::vector<std::pair<const UTrieNode *, const VarSpanNode *> > &pairVec
  = m_ruleApplications[start][end-start+1];

  MatchCallback matchCB(range, outColl);
  for (std::vector<std::pair<const UTrieNode *, const VarSpanNode *> >::const_iterator p = pairVec.begin(); p != pairVec.end(); ++p) {
    const UTrieNode &ruleNode = *(p->first);
    const VarSpanNode &varSpanNode = *(p->second);

    const UTrieNode::LabelMap &labelMap = ruleNode.GetLabelMap();

    if (varSpanNode.m_rank == 0) {  // Purely lexical rule.
      assert(labelMap.size() == 1);
      TargetPhraseCollection::shared_ptr tpc = labelMap.begin()->second;
      matchCB.m_tpc = tpc;
      matchCB(m_emptyStackVec);
    } else {  // Rule has at least one non-terminal.
      varSpanNode.CalculateRanges(start, end, m_ranges);
      m_latticeBuilder.Build(start, end, ruleNode, varSpanNode, m_ranges,
                             *this, m_lattice,
                             m_quickCheckTable);
      StackLatticeSearcher<MatchCallback> searcher(m_lattice, m_ranges);
      UTrieNode::LabelMap::const_iterator p = labelMap.begin();
      for (; p != labelMap.end(); ++p) {
        const std::vector<int> &labels = p->first;
        TargetPhraseCollection::shared_ptr tpc = p->second;
        assert(labels.size() == varSpanNode.m_rank);
        bool failCheck = false;
        for (size_t i = 0; i < varSpanNode.m_rank; ++i) {
          if (!m_quickCheckTable[i][labels[i]]) {
            failCheck = true;
            break;
          }
        }
        if (failCheck) {
          continue;
        }
        matchCB.m_tpc = tpc;
        searcher.Search(labels, matchCB);
      }
    }
  }
}

void Scope3Parser::Init()
{
  InitRuleApplicationVector();

  // Build a map from Words to index-sets.
  SentenceMap sentMap;
  FillSentenceMap(sentMap);

  // Build a trie containing 'elastic' application contexts
  const UTrieNode &rootNode = m_ruleTable.GetRootNode();
  std::auto_ptr<ApplicableRuleTrie> art(new ApplicableRuleTrie(-1, -1, rootNode));
  art->Extend(rootNode, -1, sentMap, false);

  // Build a trie containing just the non-terminal contexts and insert pointers
  // to its nodes back into the ART trie.  Contiguous non-terminal contexts are
  // merged and the number of split points is recorded.
  VarSpanTrieBuilder vstBuilder;
  m_varSpanTrie = vstBuilder.Build(*art);

  // Fill each cell with a list of pointers to relevant ART nodes.
  AddRulesToCells(*art, std::make_pair<int, int>(-1, -1), GetParser().GetSize()-1, 0);
}

void Scope3Parser::InitRuleApplicationVector()
{
  const size_t sourceSize = GetParser().GetSize();
  m_ruleApplications.resize(sourceSize);
  for (size_t start = 0; start < sourceSize; ++start) {
    size_t maxSpan = sourceSize-start+1;
    m_ruleApplications[start].resize(maxSpan+1);
  }
}

void Scope3Parser::FillSentenceMap(SentenceMap &sentMap)
{
  for (size_t i = 0; i < GetParser().GetSize(); ++i) {
    const Word &word = GetParser().GetInputPath(i, i).GetLastWord();
    sentMap[word].push_back(i);
  }
}

void Scope3Parser::AddRulesToCells(
  const ApplicableRuleTrie &node,
  std::pair<int, int> start,
  int maxPos,
  int depth)
{
  if (depth > 0) {
    // Determine the start range for this path if not already known.
    if (start.first == -1 && start.second == -1) {
      assert(depth == 1);
      start.first = std::max(0, node.m_start);
      start.second = node.m_start;
    } else if (start.second < 0) {
      assert(depth > 1);
      if (node.m_start == -1) {
        --start.second;  // Record split point
      } else {
        int numSplitPoints = -1 - start.second;
        start.second = node.m_start - (numSplitPoints+1);
      }
    }
  }

  if (node.m_node->HasRules()) {
    assert(depth > 0);
    assert(node.m_vstNode);
    // Determine the end range for this path.
    std::pair<int, int> end;
    if (node.m_end == -1) {
      end.first = (*(node.m_vstNode->m_label))[2];
      end.second = (*(node.m_vstNode->m_label))[3];
      assert(end.first != -1);
      if (end.second == -1) {
        end.second = maxPos;
      }
    } else {
      assert(node.m_start == node.m_end);  // Should be a terminal
      end.first = end.second = node.m_start;
    }
    // Add a (rule trie node, VST node) pair for each cell in the range.
    int s2 = start.second;
    if (s2 < 0) {
      int numSplitPoints = -1 - s2;
      s2 = maxPos - numSplitPoints;
    }
    for (int i = start.first; i <= s2; ++i) {
      int e1 = std::max(i+depth-1, end.first);
      for (int j = e1; j <= end.second; ++j) {
        size_t span = j-i+1;
        assert(span >= 1);
        if (m_maxChartSpan && span > m_maxChartSpan) {
          break;
        }
        m_ruleApplications[i][span].push_back(std::make_pair(node.m_node,
                                              node.m_vstNode));
      }
    }
  }

  for (std::vector<ApplicableRuleTrie*>::const_iterator p = node.m_children.begin(); p != node.m_children.end(); ++p) {
    AddRulesToCells(**p, start, maxPos, depth+1);
  }
}

}  // namespace Moses
