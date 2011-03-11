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

#include "ChartRuleLookupManagerMemory.h"

#include "PhraseDictionarySCFG.h"
#include "InputType.h"
#include "ChartTranslationOptionList.h"
#include "CellCollection.h"
#include "DotChart.h"
#include "StaticData.h"
#include "NonTerminal.h"

namespace Moses
{

ChartRuleLookupManagerMemory::ChartRuleLookupManagerMemory(
  const InputType &src,
  const CellCollection &cellColl,
  const PhraseDictionarySCFG &ruleTable)
  : ChartRuleLookupManager(src, cellColl)
  , m_ruleTable(ruleTable)
{
  assert(m_dottedRuleColls.size() == 0);
  size_t sourceSize = src.GetSize();
  m_dottedRuleColls.resize(sourceSize);

  const PhraseDictionaryNodeSCFG &rootNode = m_ruleTable.GetRootNode();

  for (size_t ind = 0; ind < m_dottedRuleColls.size(); ++ind) {
#ifdef USE_BOOST_POOL
    DottedRule *initDottedRule = m_dottedRulePool.malloc();
    new (initDottedRule) DottedRule(rootNode);
#else
    DottedRule *initDottedRule = new DottedRule(rootNode);
#endif

    DottedRuleColl *dottedRuleColl = new DottedRuleColl(sourceSize - ind + 1);
    dottedRuleColl->Add(0, initDottedRule); // init rule. stores the top node in tree

    m_dottedRuleColls[ind] = dottedRuleColl;
  }
}

ChartRuleLookupManagerMemory::~ChartRuleLookupManagerMemory()
{
  RemoveAllInColl(m_dottedRuleColls);
}

void ChartRuleLookupManagerMemory::GetChartRuleCollection(
  const WordsRange &range,
  bool adhereTableLimit,
  ChartTranslationOptionList &outColl)
{
  size_t relEndPos = range.GetEndPos() - range.GetStartPos();
  size_t absEndPos = range.GetEndPos();

  // MAIN LOOP. create list of nodes of target phrases

  // get list of all rules that apply to spans at same starting position
  DottedRuleColl &dottedRuleCol = *m_dottedRuleColls[range.GetStartPos()];
  const DottedRuleList &expandableDottedRuleList = dottedRuleCol.GetExpandableDottedRuleList();

  // loop through the rules
  // (note that expandableDottedRuleList can be expanded as the loop runs 
  //  through calls to ExtendPartialRuleApplication())
  for (size_t ind = 0; ind < expandableDottedRuleList.size(); ++ind) {
    // rule we are about to extend
    const DottedRule &prevDottedRule = *expandableDottedRuleList[ind];
    // note where it was found in the prefix tree of the rule dictionary
    const PhraseDictionaryNodeSCFG &prevNode = prevDottedRule.GetLastNode();
    // look up end position of the span it covers
    const CoveredChartSpan *prevCoveredChartSpan = prevDottedRule.GetLastCoveredChartSpan();
    // we will now try to extend it, starting after where it ended
    // (note: prevCoveredChartSpan == NULL matches for the dummy rule 
    //  at root of the prefix tree)
    size_t startPos = (prevCoveredChartSpan == NULL) ? range.GetStartPos() : prevCoveredChartSpan->GetWordsRange().GetEndPos() + 1;

    // search for terminal symbol
    // (if only one more word position needs to be covered)
    if (startPos == absEndPos) {

      // look up in rule dictionary, if the current rule can be extended
      // with the source word in the last position
      const Word &sourceWord = GetSentence().GetWord(absEndPos);
      const PhraseDictionaryNodeSCFG *node = prevNode.GetChild(sourceWord);

      // if we found a new rule -> create it and add it to the list
      if (node != NULL) {
				// create the rule
#ifdef USE_BOOST_POOL
        CoveredChartSpan *newCoveredChartSpan = m_coveredChartSpanPool.malloc();
        new (newCoveredChartSpan) CoveredChartSpan(absEndPos, absEndPos, sourceWord,
                                           prevCoveredChartSpan);
        DottedRule *dottedRule = m_dottedRulePool.malloc();
        new (dottedRule) DottedRule(*node, newCoveredChartSpan);
#else
        CoveredChartSpan *newCoveredChartSpan = new CoveredChartSpan(absEndPos, absEndPos,
            sourceWord,
            prevCoveredChartSpan);
        DottedRule *dottedRule = new DottedRule(*node,
            newCoveredChartSpan);
#endif
        dottedRuleCol.Add(relEndPos+1, dottedRule);
      }
    }

    // search for non-terminals
    size_t endPos, stackInd;

    // span is already complete covered? nothing can be done
    if (startPos > absEndPos)
      continue;

    else if (startPos == range.GetStartPos() && range.GetEndPos() > range.GetStartPos()) {
      // We're at the root of the prefix tree so won't try to cover the full
      // span (i.e. we don't allow non-lexical unary rules).  However, we need
      // to match non-unary rules that begin with a non-terminal child, so we
      // do that in two steps: during this iteration we search for non-terminals
      // that cover all but the last source word in the span (there won't
      // already be running nodes for these because that would have required a
      // non-lexical unary rule match for an earlier span).  Any matches will
      // result in running nodes being appended to the list and on subsequent
      // iterations (for this same span), we'll extend them to cover the final
      // word.
      endPos = absEndPos - 1;
      stackInd = relEndPos;
    }
    else 
    {
      endPos = absEndPos;
      stackInd = relEndPos + 1;
    }


    ExtendPartialRuleApplication(prevNode, prevCoveredChartSpan, startPos,
                                 endPos, stackInd, dottedRuleCol);
  }

  // list of rules that that cover the entire span
  DottedRuleList &rules = dottedRuleCol.Get(relEndPos + 1);

  // look up target sides for the rules
  size_t rulesLimit = StaticData::Instance().GetRuleLimit();
  DottedRuleList::const_iterator iterRule;
  for (iterRule = rules.begin(); iterRule != rules.end(); ++iterRule) {
    const DottedRule &dottedRule = **iterRule;
    const PhraseDictionaryNodeSCFG &node = dottedRule.GetLastNode();
    const CoveredChartSpan *coveredChartSpan = dottedRule.GetLastCoveredChartSpan();
    assert(coveredChartSpan);

    // look up target sides
    const TargetPhraseCollection *targetPhraseCollection = node.GetTargetPhraseCollection();

    // add the fully expanded rule (with lexical target side)
    if (targetPhraseCollection != NULL) {
      outColl.Add(*targetPhraseCollection, *coveredChartSpan, adhereTableLimit, rulesLimit);
    }
  }
  outColl.CreateChartRules(rulesLimit);
}

// Given a partial rule application ending at startPos-1 and given the sets of
// source and target non-terminals covering the span [startPos, endPos],
// determines the full or partial rule applications that can be produced through
// extending the current rule application by a single non-terminal.
void ChartRuleLookupManagerMemory::ExtendPartialRuleApplication(
  const PhraseDictionaryNodeSCFG & node,
  const CoveredChartSpan *prevCoveredChartSpan,
  size_t startPos,
  size_t endPos,
  size_t stackInd,
  DottedRuleColl & dottedRuleColl)
{
  // source non-terminal labels for the remainder
  const NonTerminalSet &sourceNonTerms =
    GetSentence().GetLabelSet(startPos, endPos);

  // target non-terminal labels for the remainder
  const NonTerminalSet &targetNonTerms =
    GetCellCollection().GetConstituentLabelSet(WordsRange(startPos, endPos));

  const PhraseDictionaryNodeSCFG::NonTerminalMap & nonTermMap =
    node.GetNonTerminalMap();

  const size_t numChildren = nonTermMap.size();
  if (numChildren == 0) {
    return;
  }
  const size_t numSourceNonTerms = sourceNonTerms.size();
  const size_t numTargetNonTerms = targetNonTerms.size();
  const size_t numCombinations = numSourceNonTerms * numTargetNonTerms;

  // We can search by either:
  //   1. Enumerating all possible source-target NT pairs that are valid for
  //      the span and then searching for matching children in the node,
  // or
  //   2. Iterating over all the NT children in the node, searching
  //      for each source and target NT in the span's sets.
  // We'll do whichever minimises the number of lookups:
  if (numCombinations <= numChildren*2) {

		// loop over possible source non-terminal labels (as found in input tree)
    NonTerminalSet::const_iterator p = sourceNonTerms.begin();
    NonTerminalSet::const_iterator sEnd = sourceNonTerms.end();
    for (; p != sEnd; ++p) {
      const Word & sourceNonTerm = *p;

      // loop over possible target non-terminal labels (as found in chart)
      NonTerminalSet::const_iterator q = targetNonTerms.begin();
      NonTerminalSet::const_iterator tEnd = targetNonTerms.end();
      for (; q != tEnd; ++q) {
        const Word & targetNonTerm = *q;

        // try to match both source and target non-terminal
        const PhraseDictionaryNodeSCFG * child =
          node.GetChild(sourceNonTerm, targetNonTerm);

        // nothing found? then we are done
        if (child == NULL) {
          continue;
        }

        // create new rule
#ifdef USE_BOOST_POOL
        CoveredChartSpan *wc = m_coveredChartSpanPool.malloc();
        new (wc) CoveredChartSpan(startPos, endPos, targetNonTerm,
                              prevCoveredChartSpan);
        DottedRule *rule = m_dottedRulePool.malloc();
        new (rule) DottedRule(*child, wc);
#else
        CoveredChartSpan * wc = new CoveredChartSpan(startPos, endPos,
                                             targetNonTerm,
                                             prevCoveredChartSpan);
        DottedRule * rule = new DottedRule(*child, wc);
#endif
        dottedRuleColl.Add(stackInd, rule);
      }
    }
  } 
  else 
  {
    // loop over possible expansions of the rule
    PhraseDictionaryNodeSCFG::NonTerminalMap::const_iterator p;
    PhraseDictionaryNodeSCFG::NonTerminalMap::const_iterator end =
      nonTermMap.end();
    for (p = nonTermMap.begin(); p != end; ++p) {
      // does it match possible source and target non-terminals?
      const PhraseDictionaryNodeSCFG::NonTerminalMapKey & key = p->first;
      const Word & sourceNonTerm = key.first;
      if (sourceNonTerms.find(sourceNonTerm) == sourceNonTerms.end()) {
        continue;
      }
      const Word & targetNonTerm = key.second;
      if (targetNonTerms.find(targetNonTerm) == targetNonTerms.end()) {
        continue;
      }

      // create new rule
      const PhraseDictionaryNodeSCFG & child = p->second;
#ifdef USE_BOOST_POOL
      CoveredChartSpan *wc = m_coveredChartSpanPool.malloc();
      new (wc) CoveredChartSpan(startPos, endPos, targetNonTerm,
                            prevCoveredChartSpan);
      DottedRule *rule = m_dottedRulePool.malloc();
      new (rule) DottedRule(child, wc);
#else
      CoveredChartSpan * wc = new CoveredChartSpan(startPos, endPos,
                                           targetNonTerm,
                                           prevCoveredChartSpan);
      DottedRule * rule = new DottedRule(child, wc);
#endif
      dottedRuleColl.Add(stackInd, rule);
    }
  }
}

}  // namespace Moses
