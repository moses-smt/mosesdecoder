// $Id: PhraseDictionarySCFGChart.cpp 3797 2011-01-13 00:25:10Z pjwilliams $
// vim:tabstop=2
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

#include "PhraseDictionarySCFG.h"
#include "FactorCollection.h"
#include "InputType.h"
#include "ChartTranslationOptionList.h"
#include "CellCollection.h"
#include "DotChart.h"
#include "StaticData.h"
#include "TreeInput.h"

using namespace std;
using namespace Moses;

void PhraseDictionarySCFG::GetChartRuleCollection(ChartTranslationOptionList &outColl
																								 ,InputType const& src
																								 ,WordsRange const& range
																								 ,bool adhereTableLimit
																								 ,const CellCollection &cellColl) const
{	
	size_t relEndPos = range.GetEndPos() - range.GetStartPos();
	size_t absEndPos = range.GetEndPos();
	
	// MAIN LOOP. create list of nodes of target phrases

	ProcessedRuleColl &processedRuleCol = *m_processedRuleColls[range.GetStartPos()];
	const ProcessedRuleList &runningNodes = processedRuleCol.GetRunningNodes();
    // Note that runningNodes can be expanded as the loop runs (through calls to
    // ExtendPartialRuleApplication()).
	for (size_t ind = 0; ind < runningNodes.size(); ++ind)
	{
		const ProcessedRule &prevProcessedRule = *runningNodes[ind];
		const PhraseDictionaryNodeSCFG &prevNode = prevProcessedRule.GetLastNode();
		const WordConsumed *prevWordConsumed = prevProcessedRule.GetLastWordConsumed();
		size_t startPos = (prevWordConsumed == NULL) ? range.GetStartPos() : prevWordConsumed->GetWordsRange().GetEndPos() + 1;
		
		// search for terminal symbol
		if (startPos == absEndPos)
		{
			const Word &sourceWord = src.GetWord(absEndPos);
			const PhraseDictionaryNodeSCFG *node = prevNode.GetChild(sourceWord);
			if (node != NULL)
			{
				WordConsumed *newWordConsumed = new WordConsumed(absEndPos, absEndPos
																												 , sourceWord
																												 , prevWordConsumed);
				ProcessedRule *processedRule = new ProcessedRule(*node, newWordConsumed);
				processedRuleCol.Add(relEndPos+1, processedRule);
			}
		}
		
		// search for non-terminals
		size_t endPos, stackInd;
		if (startPos > absEndPos)
			continue;
		else if (startPos == range.GetStartPos() && range.GetEndPos() > range.GetStartPos())
		{ // start.
			endPos = absEndPos - 1;
			stackInd = relEndPos;
		}
		else
		{
			endPos = absEndPos;
			stackInd = relEndPos + 1;
		}
		
		const NonTerminalSet &sourceNonTerms =
            src.GetLabelSet(startPos, endPos);

        const NonTerminalSet &targetNonTerms =
            cellColl.GetHeadwords(WordsRange(startPos, endPos));

        ExtendPartialRuleApplication(prevNode, prevWordConsumed, startPos,
                                     endPos, stackInd, sourceNonTerms,
                                     targetNonTerms, processedRuleCol);
	}
	
	// return list of target phrases
	ProcessedRuleList &nodes = processedRuleCol.Get(relEndPos + 1);
	
	size_t rulesLimit = StaticData::Instance().GetRuleLimit();
	ProcessedRuleList::const_iterator iterNode;
	for (iterNode = nodes.begin(); iterNode != nodes.end(); ++iterNode)
	{
		const ProcessedRule &processedRule = **iterNode;
		const PhraseDictionaryNodeSCFG &node = processedRule.GetLastNode();
		const WordConsumed *wordConsumed = processedRule.GetLastWordConsumed();
		assert(wordConsumed);
		
		const TargetPhraseCollection *targetPhraseCollection = node.GetTargetPhraseCollection();
		
		if (targetPhraseCollection != NULL)
		{
			outColl.Add(*targetPhraseCollection, *wordConsumed, adhereTableLimit, rulesLimit);
		}
	}
	outColl.CreateChartRules(rulesLimit);	
}

// Given a partial rule application ending at startPos-1 and given the sets of
// source and target non-terminals covering the span [startPos, endPos],
// determines the full or partial rule applications that can be produced through
// extending the current rule application by a single non-terminal.
void PhraseDictionarySCFG::ExtendPartialRuleApplication(
    const PhraseDictionaryNodeSCFG & node,
    const WordConsumed *prevWordConsumed,
    size_t startPos,
    size_t endPos,
    size_t stackInd,
    const NonTerminalSet & sourceNonTerms,
    const NonTerminalSet & targetNonTerms,
    ProcessedRuleColl & processedRuleColl) const
{
    const PhraseDictionaryNodeSCFG::NonTerminalMap & nonTermMap =
        node.GetNonTerminalMap();

    const size_t numChildren = nonTermMap.size();
    if (numChildren == 0)
    {
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
    if (numCombinations <= numChildren*2)
    {
        NonTerminalSet::const_iterator p = sourceNonTerms.begin();
        NonTerminalSet::const_iterator sEnd = sourceNonTerms.end();
        for (; p != sEnd; ++p)
        {
            const Word & sourceNonTerm = *p;
            NonTerminalSet::const_iterator q = targetNonTerms.begin();
            NonTerminalSet::const_iterator tEnd = targetNonTerms.end();
            for (; q != tEnd; ++q)
            {
                const Word & targetNonTerm = *q;
                const PhraseDictionaryNodeSCFG * child =
                    node.GetChild(sourceNonTerm, targetNonTerm);
                if (child == NULL)
                {
                    continue;
                }
                WordConsumed * wc = new WordConsumed(startPos, endPos,
                                                     targetNonTerm,
                                                     prevWordConsumed);
                ProcessedRule * rule = new ProcessedRule(*child, wc);
                processedRuleColl.Add(stackInd, rule);
            }
        }
    }
    else
    {
        PhraseDictionaryNodeSCFG::NonTerminalMap::const_iterator p;
        PhraseDictionaryNodeSCFG::NonTerminalMap::const_iterator end =
                                                            nonTermMap.end();
        for (p = nonTermMap.begin(); p != end; ++p)
        {
            const PhraseDictionaryNodeSCFG::NonTerminalMapKey & key = p->first;
            const Word & sourceNonTerm = key.first;
            if (sourceNonTerms.find(sourceNonTerm) == sourceNonTerms.end())
            {
                continue;
            }
            const Word & targetNonTerm = key.second;
            if (targetNonTerms.find(targetNonTerm) == targetNonTerms.end())
            {
                continue;
            }
            const PhraseDictionaryNodeSCFG & child = p->second;
            WordConsumed * wc = new WordConsumed(startPos, endPos,
                                                 targetNonTerm,
                                                 prevWordConsumed);
            ProcessedRule * rule = new ProcessedRule(child, wc);
            processedRuleColl.Add(stackInd, rule);
        }
    }
}
