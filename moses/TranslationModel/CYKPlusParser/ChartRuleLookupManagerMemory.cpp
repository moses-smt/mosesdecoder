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

#include <iostream>
#include "ChartRuleLookupManagerMemory.h"

#include "moses/ChartParser.h"
#include "moses/InputType.h"
#include "moses/Terminal.h"
#include "moses/ChartParserCallback.h"
#include "moses/StaticData.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellCollection.h"
#include "moses/FactorCollection.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"

using namespace std;

namespace Moses
{

ChartRuleLookupManagerMemory::ChartRuleLookupManagerMemory(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellColl,
  const PhraseDictionaryMemory &ruleTable)
  : ChartRuleLookupManagerCYKPlus(parser, cellColl)
  , m_ruleTable(ruleTable)
  , m_softMatchingMap(StaticData::Instance().GetSoftMatches())
{

  size_t sourceSize = parser.GetSize();
  size_t ruleLimit  = parser.options()->syntax.rule_limit;
  m_completedRules.resize(sourceSize, CompletedRuleCollection(ruleLimit));

  m_isSoftMatching = !m_softMatchingMap.empty();
}

void ChartRuleLookupManagerMemory::GetChartRuleCollection(
  const InputPath &inputPath,
  size_t lastPos,
  ChartParserCallback &outColl)
{
  const Range &range = inputPath.GetWordsRange();
  size_t startPos = range.GetStartPos();
  size_t absEndPos = range.GetEndPos();

  m_lastPos = lastPos;
  m_stackVec.clear();
  m_stackScores.clear();
  m_outColl = &outColl;
  m_unaryPos = absEndPos-1; // rules ending in this position are unary and should not be added to collection

  // create/update data structure to quickly look up all chart cells that match start position and label.
  UpdateCompressedMatrix(startPos, absEndPos, lastPos);

  const PhraseDictionaryNodeMemory &rootNode = m_ruleTable.GetRootNode();

  // all rules starting with terminal
  if (startPos == absEndPos) {
    GetTerminalExtension(&rootNode, startPos);
  }
  // all rules starting with nonterminal
  else if (absEndPos > startPos) {
    GetNonTerminalExtension(&rootNode, startPos);
  }

  // copy temporarily stored rules to out collection
  CompletedRuleCollection & rules = m_completedRules[absEndPos];
  for (vector<CompletedRule*>::const_iterator iter = rules.begin(); iter != rules.end(); ++iter) {
    outColl.Add((*iter)->GetTPC(), (*iter)->GetStackVector(), range);
  }

  rules.Clear();

}

// Create/update compressed matrix that stores all valid ChartCellLabels for a given start position and label.
void ChartRuleLookupManagerMemory::UpdateCompressedMatrix(size_t startPos,
    size_t origEndPos,
    size_t lastPos)
{

  std::vector<size_t> endPosVec;
  size_t numNonTerms = FactorCollection::Instance().GetNumNonTerminals();
  m_compressedMatrixVec.resize(lastPos+1);

  // we only need to update cell at [startPos, origEndPos-1] for initial lookup
  if (startPos < origEndPos) {
    endPosVec.push_back(origEndPos-1);
  }

  // update all cells starting from startPos+1 for lookup of rule extensions
  else if (startPos == origEndPos) {
    startPos++;
    for (size_t endPos = startPos; endPos <= lastPos; endPos++) {
      endPosVec.push_back(endPos);
    }
    //re-use data structure for cells with later start position, but remove chart cells that would break max-chart-span
    for (size_t pos = startPos+1; pos <= lastPos; pos++) {
      CompressedMatrix & cellMatrix = m_compressedMatrixVec[pos];
      cellMatrix.resize(numNonTerms);
      for (size_t i = 0; i < numNonTerms; i++) {
        if (!cellMatrix[i].empty() && cellMatrix[i].back().endPos > lastPos) {
          cellMatrix[i].pop_back();
        }
      }
    }
  }

  if (startPos > lastPos) {
    return;
  }

  // populate compressed matrix with all chart cells that start at current start position
  CompressedMatrix & cellMatrix = m_compressedMatrixVec[startPos];
  cellMatrix.clear();
  cellMatrix.resize(numNonTerms);
  for (std::vector<size_t>::iterator p = endPosVec.begin(); p != endPosVec.end(); ++p) {

    size_t endPos = *p;
    // target non-terminal labels for the span
    const ChartCellLabelSet &targetNonTerms = GetTargetLabelSet(startPos, endPos);

    if (targetNonTerms.GetSize() == 0) {
      continue;
    }

#if !defined(UNLABELLED_SOURCE)
    // source non-terminal labels for the span
    const InputPath &inputPath = GetParser().GetInputPath(startPos, endPos);

    // can this ever be true? Moses seems to pad the non-terminal set of the input with [X]
    if (inputPath.GetNonTerminalSet().size() == 0) {
      continue;
    }
#endif

    for (size_t i = 0; i < numNonTerms; i++) {
      const ChartCellLabel *cellLabel = targetNonTerms.Find(i);
      if (cellLabel != NULL) {
        float score = cellLabel->GetBestScore(m_outColl);
        cellMatrix[i].push_back(ChartCellCache(endPos, cellLabel, score));
      }
    }
  }
}

// if a (partial) rule matches, add it to list completed rules (if non-unary and non-empty), and try find expansions that have this partial rule as prefix.
void ChartRuleLookupManagerMemory::AddAndExtend(
  const PhraseDictionaryNodeMemory *node,
  size_t endPos)
{

  TargetPhraseCollection::shared_ptr tpc = node->GetTargetPhraseCollection();
  // add target phrase collection (except if rule is empty or a unary non-terminal rule)
  if (!tpc->IsEmpty() && (m_stackVec.empty() || endPos != m_unaryPos)) {
    m_completedRules[endPos].Add(*tpc, m_stackVec, m_stackScores, *m_outColl);
  }

  // get all further extensions of rule (until reaching end of sentence or max-chart-span)
  if (endPos < m_lastPos) {
    if (!node->GetTerminalMap().empty()) {
      GetTerminalExtension(node, endPos+1);
    }
    if (!node->GetNonTerminalMap().empty()) {
      GetNonTerminalExtension(node, endPos+1);
    }
  }
}


// search all possible terminal extensions of a partial rule (pointed at by node) at a given position
// recursively try to expand partial rules into full rules up to m_lastPos.
void ChartRuleLookupManagerMemory::GetTerminalExtension(
  const PhraseDictionaryNodeMemory *node,
  size_t pos)
{

  const Word &sourceWord = GetSourceAt(pos).GetLabel();
  const PhraseDictionaryNodeMemory::TerminalMap & terminals = node->GetTerminalMap();

  // if node has small number of terminal edges, test word equality for each.
  if (terminals.size() < 5) {
    for (PhraseDictionaryNodeMemory::TerminalMap::const_iterator iter = terminals.begin(); iter != terminals.end(); ++iter) {
      const Word & word = iter->first;
      if (TerminalEqualityPred()(word, sourceWord)) {
        const PhraseDictionaryNodeMemory *child = & iter->second;
        AddAndExtend(child, pos);
        break;
      }
    }
  }
  // else, do hash lookup
  else {
    const PhraseDictionaryNodeMemory *child = node->GetChild(sourceWord);
    if (child != NULL) {
      AddAndExtend(child, pos);
    }
  }
}

// search all nonterminal possible nonterminal extensions of a partial rule (pointed at by node) for a variable span (starting from startPos).
// recursively try to expand partial rules into full rules up to m_lastPos.
void ChartRuleLookupManagerMemory::GetNonTerminalExtension(
  const PhraseDictionaryNodeMemory *node,
  size_t startPos)
{

  const CompressedMatrix &compressedMatrix = m_compressedMatrixVec[startPos];

  // non-terminal labels in phrase dictionary node
  const PhraseDictionaryNodeMemory::NonTerminalMap & nonTermMap = node->GetNonTerminalMap();

  // make room for back pointer
  m_stackVec.push_back(NULL);
  m_stackScores.push_back(0);

  // loop over possible expansions of the rule
  PhraseDictionaryNodeMemory::NonTerminalMap::const_iterator p;
  PhraseDictionaryNodeMemory::NonTerminalMap::const_iterator end = nonTermMap.end();
  for (p = nonTermMap.begin(); p != end; ++p) {
    // does it match possible source and target non-terminals?
#if defined(UNLABELLED_SOURCE)
    const Word &targetNonTerm = p->first;
#else
    const Word &targetNonTerm = p->first.second;
#endif
    const PhraseDictionaryNodeMemory *child = &p->second;
    //soft matching of NTs
    if (m_isSoftMatching && !m_softMatchingMap[targetNonTerm[0]->GetId()].empty()) {
      const std::vector<Word>& softMatches = m_softMatchingMap[targetNonTerm[0]->GetId()];
      for (std::vector<Word>::const_iterator softMatch = softMatches.begin(); softMatch != softMatches.end(); ++softMatch) {
        const CompressedColumn &matches = compressedMatrix[(*softMatch)[0]->GetId()];
        for (CompressedColumn::const_iterator match = matches.begin(); match != matches.end(); ++match) {
          m_stackVec.back() = match->cellLabel;
          m_stackScores.back() = match->score;
          AddAndExtend(child, match->endPos);
        }
      }
    } // end of soft matches lookup

    const CompressedColumn &matches = compressedMatrix[targetNonTerm[0]->GetId()];
    for (CompressedColumn::const_iterator match = matches.begin(); match != matches.end(); ++match) {
      m_stackVec.back() = match->cellLabel;
      m_stackScores.back() = match->score;
      AddAndExtend(child, match->endPos);
    }
  }
  // remove last back pointer
  m_stackVec.pop_back();
  m_stackScores.pop_back();
}

}  // namespace Moses
