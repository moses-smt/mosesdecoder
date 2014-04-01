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
#include "ChartRuleLookupManagerMemoryMBOT.h"

#include "PhraseDictionaryMBOT.h"
#include "InputType.h"
#include "ChartTranslationOptionList.h"
#include "CellCollection.h"
#include "DotChartInMemory.h"
#include "StaticData.h"
#include "NonTerminal.h"
#include "ChartCellCollection.h"
#include "ChartTranslationOptionCollection.h"

namespace Moses
{

ChartRuleLookupManagerMemoryMBOT::ChartRuleLookupManagerMemoryMBOT(
  const InputType &src,
  const ChartCellCollection &cellColl,
  const PhraseDictionaryMBOT &ruleTable)
  : ChartRuleLookupManagerMemory(src, cellColl, ruleTable)
  , m_mbotRuleTable(ruleTable)
{

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

void ChartRuleLookupManagerMemoryMBOT::GetChartRuleCollection(
  const WordsRange &range,
  bool adhereTableLimit,
  ChartTranslationOptionList &outColl)
{
  size_t relEndPos = range.GetEndPos() - range.GetStartPos();
  size_t absEndPos = range.GetEndPos();
  // MAIN LOOP. create list of nodes of target phrases

  // get list of all rules that apply to spans at same starting position
  DottedRuleCollMBOT &dottedRuleCol = *m_mbotDottedRuleColls[range.GetStartPos()];
  const DottedRuleListMBOT &expandableDottedRuleList = dottedRuleCol.GetExpandableDottedRuleListMBOT();

  //Get input words : strings
  const ChartCellLabelMBOT &sourceWordLabel = GetCellCollection().GetMBOT(WordsRange(absEndPos, absEndPos)).GetSourceWordLabel();

  // loop through the rules
  // (note that expandableDottedRuleList can be expa    nded as the loop runs
  //  through calls to ExtendPartialRuleApplication())
  for (size_t ind = 0; ind < expandableDottedRuleList.size(); ++ind) {
    // rule we are about to extend
    const DottedRuleInMemoryMBOT * prevDottedRule = expandableDottedRuleList[ind];

    // we will now try to extend it, starting after where it ended
    size_t startPos = prevDottedRule->IsRootMBOT()
                    ? range.GetStartPos()
                    : prevDottedRule->GetWordsRangeMBOT().GetEndPos() + 1;

    // search for terminal symbol
    // (if only one more word position needs to be covered)
    if (startPos == absEndPos) {

      // look up in rule dictionary, if the current rule can be extended
      // with the source word in the last position
      //take first element of MBOT label for source word

      //source word label is MBOT
      CHECK(sourceWordLabel.GetLabelMBOT().size() == 1);
      const Word &sourceWord = sourceWordLabel.GetLabelMBOT().front();
      const PhraseDictionaryNodeMBOT *node = prevDottedRule->GetLastNode().GetChildMBOT(sourceWord);

     // if we found a new rule -> create it and add it to the list
      if (node != NULL) {
			// create the rule
#ifdef USE_BOOST_POOL
        DottedRuleInMemoryMBOT *dottedRule = m_dottedRulePool.malloc();
        new (dottedRule) DottedRuleInMemoryMBOT(*node, sourceWordLabel,
                                            prevDottedRule);
#else
        DottedRuleInMemoryMBOT *dottedRule = new DottedRuleInMemoryMBOT(*node,
                                                                sourceWordLabel,
                                                                *prevDottedRule);
#endif
        dottedRuleCol.Add(relEndPos+1, dottedRule);
      }
    }

    // search for non-terminals
    size_t endPos, stackInd;

    // span is already complete covered? nothing can be done
    if (startPos > absEndPos){
      continue;}

    else if (startPos == range.GetStartPos() && range.GetEndPos() > range.GetStartPos()) {
    //std::cout << " n : n : Processing Root of Prefix Tree..." << std::endl;
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
    ExtendPartialRuleApplicationMBOT(*prevDottedRule, startPos, endPos, stackInd,
                                 dottedRuleCol);
  }

  // list of rules that that cover the entire span
  DottedRuleListMBOT &rules = dottedRuleCol.GetMBOT(relEndPos + 1);
  // look up target sides for the rules
  size_t rulesLimit = StaticData::Instance().GetRuleLimit();
  DottedRuleListMBOT::const_iterator iterRule;
  for (iterRule = rules.begin(); iterRule != rules.end(); ++iterRule) {
    const DottedRuleInMemoryMBOT &dottedRuleMBOT = **iterRule;
    const PhraseDictionaryNodeMBOT &node = dottedRuleMBOT.GetLastNode();
    // look up target sides
    const TargetPhraseCollection *targetPhraseCollection = node.GetTargetPhraseCollectionMBOT();

     // add the fully expanded rule (with lexical target side)
    if (targetPhraseCollection != NULL) {
     DottedRuleInMemoryMBOT * dottedRuleNotConst = const_cast<DottedRuleInMemoryMBOT*>(&dottedRuleMBOT);
     DottedRuleMBOT *dottedRuleForColl = static_cast<DottedRuleMBOT*>(dottedRuleNotConst);
      outColl.AddMBOT(*targetPhraseCollection, *dottedRuleForColl,
                  GetCellCollection(), adhereTableLimit, rulesLimit);
    }
  }
  dottedRuleCol.Clear(relEndPos+1);
  outColl.CreateChartRules(rulesLimit);
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

    const NonTerminalSet &sourceNonTerms = GetSentence().GetLabelSet(startPos, endPos);
    NonTerminalSet allSourceNonTerms;
    NonTerminalSet::iterator itr_nonTermMap;

    std::vector<size_t> inputFactors;
    if(StaticData::Instance().GetSourceNonTerminals().size() > 0)
    {
    	 std::cerr << "Number of input non-terminals : " << StaticData::Instance().GetSourceNonTerminals().size() << std::endl;
    	 std::vector<std::string>::const_iterator itr_source_terms;
    	 for(itr_source_terms = StaticData::Instance().GetSourceNonTerminals().begin();itr_source_terms != StaticData::Instance().GetSourceNonTerminals().end();itr_source_terms++)
    	 {
    		 Word sourceNonTerm;
    		 sourceNonTerm.CreateFromString(Input,inputFactors, *itr_source_terms, true);
    		 allSourceNonTerms.insert(sourceNonTerm);
		}

    	 //also insert labels of current sentence to make sure we have everything
    	 for(itr_nonTermMap = sourceNonTerms.begin(); itr_nonTermMap != sourceNonTerms.end(); itr_nonTermMap++)
    	 {
    	     Word sourceWord =  *itr_nonTermMap;
    	     allSourceNonTerms.insert(sourceWord);
    	 }
    }


  // target non-terminal labels for the remainder
    const ChartCellLabelSetMBOT &targetNonTerms =
    GetCellCollection().GetMBOT(WordsRange(startPos, endPos)).GetTargetLabelSet();
    ChartCellLabelSetMBOT::const_iterator my_iter_target;
    for(my_iter_target = targetNonTerms.begin(); my_iter_target != targetNonTerms.end(); my_iter_target++)
    {
      const ChartCellLabelMBOT &cellLabel = *my_iter_target;
    }

  // note where it was found in the prefix tree of the rule dictionary
  const PhraseDictionaryNodeMBOT &node = prevDottedRule.GetLastNode();
  const PhraseDictionaryNodeMBOT::NonTerminalMapMBOT & nonTermMap =
    node.GetNonTerminalMapMBOT();
  const size_t numChildren = nonTermMap.size();

  if (numChildren == 0) {
    return;
  }

  const size_t numSourceNonTerms = sourceNonTerms.size();
  const size_t numTargetNonTerms = targetNonTerms.GetSizeMBOT();
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
      ChartCellLabelSetMBOT::const_iterator q = targetNonTerms.begin();
      ChartCellLabelSetMBOT::const_iterator tEnd = targetNonTerms.end();
      for (; q != tEnd; ++q) {
        const ChartCellLabelMBOT &cellLabel = *q;
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
    for (p = nonTermMap.begin(); p != end; ++p) {
      // does it match possible source and target non-terminals?
      const PhraseDictionaryNodeMBOT::NonTerminalMapKeyMBOT &key = p->first;
      const Word &sourceNonTerm = key.first;
      if (sourceNonTerms.find(sourceNonTerm) == sourceNonTerms.end()) {
        continue;
      }
      const std::vector<Word> &targetNonTerm = key.second;
      const ChartCellLabelMBOT *cellLabel = targetNonTerms.FindMBOT(targetNonTerm);


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
