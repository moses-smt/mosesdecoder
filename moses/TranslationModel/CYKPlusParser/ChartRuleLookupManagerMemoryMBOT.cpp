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


#include "ChartRuleLookupManagerMemoryMBOT.h"
#include "DotChartInMemoryMBOT.h"

#include "moses/TranslationModel/RuleTable/PhraseDictionaryMBOT.h"
#include "moses/InputType.h"
#include "moses/ChartParserCallback.h"
#include "moses/StaticData.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellCollection.h"


namespace Moses
{

ChartRuleLookupManagerMemoryMBOT::ChartRuleLookupManagerMemoryMBOT(
  const InputType &src,
  const ChartCellCollectionBase &cellColl,
  const PhraseDictionaryMBOT &ruleTable)
  : ChartRuleLookupManagerMemory(src, cellColl, ruleTable)
  , ChartRuleLookupManagerShallowMBOT(src,cellColl)
  , m_mbotRuleTable(ruleTable)
{

  //std::cout << "BEGIN CREATING DOTTED RULES FOR MBOT" << std::endl;

  CHECK(m_mbotDottedRuleColls.size() == 0);
  size_t sourceSize = src.GetSize();
  m_mbotDottedRuleColls.resize(sourceSize);

  const PhraseDictionaryNodeMBOT &rootNode = m_mbotRuleTable.GetRootNodeMBOT();

  for (size_t ind = 0; ind < m_mbotDottedRuleColls.size(); ++ind) {
#ifdef USE_BOOST_POOL
    DottedRuleInMemoryMBOT *initDottedRule = m_mbotDottedRulePool.malloc();
    new (initDottedRule) DottedRuleInMemoryMBOT(rootNode);
#else
    DottedRuleInMemoryMBOT *initDottedRule = new DottedRuleInMemoryMBOT(rootNode);
#endif

    DottedRuleCollMBOT *dottedRuleColl = new DottedRuleCollMBOT(sourceSize - ind + 1);
    dottedRuleColl->Add(0, initDottedRule); // init rule. stores the top node in tree

    m_mbotDottedRuleColls[ind] = dottedRuleColl;
  }
}

ChartRuleLookupManagerMemoryMBOT::~ChartRuleLookupManagerMemoryMBOT()
{
  RemoveAllInColl(m_mbotDottedRuleColls);
}

void ChartRuleLookupManagerMemoryMBOT::GetMBOTRuleCollection(
  const WordsRange &range,
  ChartParserCallback &outColl)
{

	std::cerr << "GETTING MBOT RULE COLLECTION... " << std::endl;

  size_t relEndPos = range.GetEndPos() - range.GetStartPos();
  size_t absEndPos = range.GetEndPos();

  // get list of all rules that apply to spans at same starting position
  DottedRuleCollMBOT &dottedRuleCol = *m_mbotDottedRuleColls[range.GetStartPos()];

  std::cerr << "GOT RULE COLLECTION... " << std::endl;

  const DottedRuleListMBOT &expandableDottedRuleList = dottedRuleCol.GetExpandableDottedRuleListMBOT();
  std::cerr << "BEFORE LOOKING FOR WORD" << std::endl;

  //std::cerr << "LOOKING FOR WORD : " << *(GetMBOTSourceWordAt(absEndPos,0)) << std::endl;

  //Exandable list : in which leaves are non-terminals
  for (size_t ind = 0; ind < expandableDottedRuleList.size(); ++ind) {
    const DottedRuleInMemoryMBOT * prevDottedRule = expandableDottedRuleList[ind];

    // we will now try to extend it, starting after where it ended
    size_t startPos = prevDottedRule->IsRootMBOT()
                    ? range.GetStartPos()
                    : prevDottedRule->GetWordsRangeMBOT().GetEndPos() + 1;
    if (startPos == absEndPos) {

      CHECK(GetMBOTSizeAt(absEndPos) == 1);
      //Fabienne Braune : when working with source language side, we only need one chart label so we take the
      //first element of the l-MBOT-ChartCellLabels;
      const PhraseDictionaryNodeMBOT *node = prevDottedRule->GetLastNode().GetChildMBOT(GetMBOTSourceWordAt(absEndPos,0));

      if (node != NULL) {
#ifdef USE_BOOST_POOL
        DottedRuleInMemoryMBOT *dottedRule = m_dottedRulePool.malloc();
        new (dottedRule) DottedRuleInMemoryMBOT(*node, sourceWordLabel,
#else
        DottedRuleInMemoryMBOT *dottedRule = new DottedRuleInMemoryMBOT(*node,
        											*GetMBOTSourceAt(absEndPos),
                                                                *prevDottedRule);
#endif
        std::cerr << "Node found, adding to collection ... " << std::endl;
        dottedRuleCol.Add(relEndPos+1, dottedRule);
      }
    }

    // search for non-terminals
    size_t endPos, stackInd;

    // span is already complete covered? nothing can be done
    if (startPos > absEndPos){
      continue;}

    else if (startPos == range.GetStartPos() && range.GetEndPos() > range.GetStartPos()) {
      endPos = absEndPos - 1;
      stackInd = relEndPos;
    }
    else
    {
      endPos = absEndPos;
      stackInd = relEndPos + 1;
    }
    ExtendPartialRuleApplicationMBOT(*prevDottedRule, startPos, endPos, stackInd,
                                 dottedRuleCol);
  }

  DottedRuleListMBOT &rules = dottedRuleCol.GetMBOT(relEndPos + 1);
  size_t rulesLimit = StaticData::Instance().GetRuleLimit();
  DottedRuleListMBOT::const_iterator iterRule;

  for (iterRule = rules.begin(); iterRule != rules.end(); ++iterRule) {

	const DottedRuleInMemoryMBOT &dottedRuleMBOT = **iterRule;
    const PhraseDictionaryNodeMBOT &node = dottedRuleMBOT.GetLastNode();
    const TargetPhraseCollection *tpc = node.GetTargetPhraseCollectionMBOT();

    if (tpc != NULL) {
    		AddCompletedRuleMBOT(dottedRuleMBOT, *tpc, range, outColl);
    }
  }
  dottedRuleCol.Clear(relEndPos+1);
}

// Given a partial rule application ending at startPos-1 and given the sets of
// source and target non-terminals covering the span [startPos, endPos],
// determines the full or partial rule applications that can be produced through
// extending the current rule application by a single non-terminal.
void ChartRuleLookupManagerMemoryMBOT::ExtendPartialRuleApplicationMBOT(
  const DottedRuleInMemoryMBOT &prevDottedRule,
  size_t startPos,
  size_t endPos,
  size_t stackInd,
  DottedRuleCollMBOT & dottedRuleColl)
{

    int debugLevel = 1;

    const NonTerminalSet &sourceNonTerms =
        GetSentenceFromMBOT().GetLabelSet(startPos, endPos);

    const ChartCellLabelSetMBOT &targetNonTerms = GetMBOTTargetLabelSet(startPos, endPos);

    const PhraseDictionaryNodeMBOT &node = prevDottedRule.GetLastNode();

    const PhraseDictionaryNodeMBOT::NonTerminalMapMBOT & nonTermMap = node.GetNonTerminalMapMBOT();

    const size_t numChildren = nonTermMap.size();

    if (numChildren == 0) {
    return;
  }
  const size_t numSourceNonTerms = sourceNonTerms.size();
  const size_t numTargetNonTerms = targetNonTerms.GetSize();
  const size_t numCombinations = numSourceNonTerms * numTargetNonTerms;

  if (numCombinations <= numChildren*2) {

    NonTerminalSet::const_iterator p = sourceNonTerms.begin();
    NonTerminalSet::const_iterator sEnd = sourceNonTerms.end();
    for (; p != sEnd; ++p) {
      const Word & sourceNonTerm = *p;

      ChartCellLabelSetMBOT::const_iterator q = targetNonTerms.begin();
      ChartCellLabelSetMBOT::const_iterator tEnd = targetNonTerms.end();
      for (; q != tEnd; ++q) {

        const ChartCellLabelMBOT &cellLabel = q->second;

        // try to match both source and target non-terminal
        const PhraseDictionaryNodeMBOT * child =
          node.GetChild(sourceNonTerm, cellLabel.GetLabelMBOT());

        // nothing found? then we are done
        if (child == NULL) {
          continue;
        }

        // create new rule
#ifdef USE_BOOST_POOL
        DottedRuleInMemory *rule = m_dottedRulePool.malloc();
        new (rule) DottedRuleInMemoryMBOT(*child, cellLabel, prevDottedRule);
#else
        DottedRuleInMemoryMBOT *rule = new DottedRuleInMemoryMBOT(*child, cellLabel,
                                                          prevDottedRule);

        rule->SetSourceLabel(sourceNonTerm);
#endif
        dottedRuleColl.Add(stackInd, rule);
      }
    }
  }
  else
  {
    // loop over possible expansions of the rule
    PhraseDictionaryNodeMBOT::NonTerminalMapMBOT::const_iterator p;
    PhraseDictionaryNodeMBOT::NonTerminalMapMBOT::const_iterator end =
      nonTermMap.end();
    //std::cout << "Iterating over non terminal map :" << std::endl;
    for (p = nonTermMap.begin(); p != end; ++p) {
      // does it match possible source and target non-terminals?
      const PhraseDictionaryNodeMBOT::NonTerminalMapKeyMBOT &key = p->first;
      const Word &sourceNonTerm = key.first;
      if (sourceNonTerms.find(sourceNonTerm) == sourceNonTerms.end()) {
        continue;
      }
      const WordSequence &targetNonTerm = key.second;
      const ChartCellLabelMBOT *cellLabel = targetNonTerms.Find(targetNonTerm);


      if (!cellLabel) {
        continue;
      }

      // create new rule
      const PhraseDictionaryNodeMBOT &child = p->second;
#ifdef USE_BOOST_POOL
      DottedRuleInMemoryMBOT *rule = m_dottedRulePool.malloc();
      new (rule) DottedRuleInMemoryMBOT(child, *cellLabel, prevDottedRule);
#else
      DottedRuleInMemoryMBOT *rule = new DottedRuleInMemoryMBOT(child, *cellLabel,
                                                        prevDottedRule);
#endif

    //new : put source info into rule
      rule->SetSourceLabel(sourceNonTerm);
      dottedRuleColl.Add(stackInd, rule);
    }
  }
}

}  // namespace Moses
