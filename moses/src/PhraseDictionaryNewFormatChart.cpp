
#include "PhraseDictionaryNewFormat.h"
#include "FactorCollection.h"
#include "InputType.h"
#include "ChartRuleCollection.h"
#include "CellCollection.h"
#include "DotChart.h"
#include "StaticData.h"
#include "ChartInput.h"

using namespace std;
using namespace Moses;

Word PhraseDictionaryNewFormat::CreateCoveredWord(const Word &origSourceLabel, const InputType &src, const WordsRange &range) const
{
	string coveredWordsString = origSourceLabel.GetFactor(0)->GetString();
	
	for (size_t pos = range.GetStartPos(); pos <= range.GetEndPos(); ++pos)
	{
		const Word &word = src.GetWord(pos);
		coveredWordsString += "_" + word.GetFactor(0)->GetString();
	}
	
	FactorCollection &factorCollection = FactorCollection::Instance();
	
	Word ret;
	
	const Factor *factor = factorCollection.AddFactor(Input, 0, coveredWordsString, true);
	ret.SetFactor(0, factor);
	
	return ret;
}

const ChartRuleCollection *PhraseDictionaryNewFormat::GetChartRuleCollection(InputType const& src
																																						 , WordsRange const& range
																																						 , bool adhereTableLimit
																																						 , const CellCollection &cellColl
																																						 , size_t maxDefaultSpan
																																						 , size_t maxSourceSyntaxSpan
																																						 , size_t maxTargetSyntaxSpan) const
{
	const StaticData &staticData = StaticData::Instance();
	
	ChartRuleCollection *ret = new ChartRuleCollection();
	m_chartTargetPhraseColl.push_back(ret);
	
	size_t relEndPos = range.GetEndPos() - range.GetStartPos();
	size_t absEndPos = range.GetEndPos();
	
	// MAIN LOOP. create list of nodes of target phrases
	ProcessedRuleStack &runningNodes = *m_runningNodesVec[range.GetStartPos()];
	
	const ProcessedRuleStack::SavedNodeColl &savedNodeColl = runningNodes.GetSavedNodeColl();
	for (size_t ind = 0; ind < savedNodeColl.size(); ++ind)
	{
		const SavedNode &savedNode = *savedNodeColl[ind];
		const ProcessedRule &prevProcessedRule = savedNode.GetProcessedRule();
		const PhraseDictionaryNodeSourceLabel &prevNode = static_cast<const PhraseDictionaryNodeSourceLabel &>(prevProcessedRule.GetLastNode());
		const WordConsumed *prevWordConsumed = prevProcessedRule.GetLastWordConsumed();
		size_t startPos = (prevWordConsumed == NULL) ? range.GetStartPos() : prevWordConsumed->GetWordsRange().GetEndPos() + 1;
		
		// search for terminal symbol
		if (startPos == absEndPos)
		{
			const Word &sourceWord = src.GetWord(absEndPos);
			const PhraseDictionaryNodeSourceLabel *node = prevNode.GetChild(sourceWord, sourceWord);
			if (node != NULL)
			{
				const Word &sourceWord = node->GetSourceWord();
				WordConsumed *newWordConsumed = new WordConsumed(absEndPos, absEndPos
																												 , sourceWord, NULL
																												 , prevWordConsumed);
				ProcessedRule *processedRule = new ProcessedRule(*node, newWordConsumed);
				runningNodes.Add(relEndPos+1, processedRule);
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
		
		size_t nonTermNumWordsCovered = endPos - startPos + 1;
		
		// get target lhs in this span from chart
		const vector<const Word*> headWords = cellColl.GetTargetLHSList(WordsRange(startPos, endPos));
		
		const Word &defaultSourceNonTerm = staticData.GetInputDefaultNonTerminal()
							,&defaultTargetNonTerm = staticData.GetOutputDefaultNonTerminal();
		
		// go thru each source span
		const ChartInput &chartInput = static_cast<const ChartInput&>(src);
		const LabelList &sourceLHSList = chartInput.GetLabelList(startPos, endPos);
		
		LabelList::const_iterator iterSourceLHS;
		for (iterSourceLHS = sourceLHSList.begin(); iterSourceLHS != sourceLHSList.end(); ++iterSourceLHS)
		{
			const Word &sourceLHS = *iterSourceLHS;
			
			bool isSourceSyntaxNonTerm = (sourceLHS != defaultSourceNonTerm);
			size_t maxSpan = max(maxDefaultSpan, isSourceSyntaxNonTerm ? maxSourceSyntaxSpan:0);

			// go thru each target LHS & see if in phrase table
			vector<const Word*>::const_iterator iterTargetLHS;
			for (iterTargetLHS = headWords.begin(); iterTargetLHS != headWords.end(); ++iterTargetLHS)
			{
				const Word &targetLHS = **iterTargetLHS;
				
				//cerr << sourceLHS << " " << defaultSourceNonTerm << " " << targetLHS << " " << defaultTargetNonTerm << endl;
				
				bool isTargetSyntaxNonTerm = (targetLHS != defaultTargetNonTerm);
				maxSpan = max(maxSpan, isTargetSyntaxNonTerm ? maxTargetSyntaxSpan:0);
				bool doSearch = nonTermNumWordsCovered <=  maxSpan;

				if (doSearch)
				{
					const PhraseDictionaryNodeSourceLabel *node = prevNode.GetChild(targetLHS, sourceLHS);
					if (node != NULL)
					{
						//const Word &sourceWord = node->GetSourceWord();
						WordConsumed *newWordConsumed = new WordConsumed(startPos, endPos
																														 , targetLHS, &sourceLHS
																														 , prevWordConsumed);
						
						ProcessedRule *processedRule = new ProcessedRule(*node, newWordConsumed);
						runningNodes.Add(stackInd, processedRule);
					}
					else
					{
						//cerr << range;
					}
					
				} // do search
			} // for (iterHeadWords
		} // for (iterLabelList 
	}
	
	// return list of target phrases
	ProcessedRuleColl &nodes = runningNodes.Get(relEndPos + 1);
	//DeleteDuplicates(nodes);

	size_t rulesLimit = StaticData::Instance().GetRuleLimit();

	// source LHS
	ProcessedRuleColl::const_iterator iterProcessedRuleColl;
	for (iterProcessedRuleColl = nodes.begin(); iterProcessedRuleColl != nodes.end(); ++iterProcessedRuleColl)
	{
		// node of last source word
		const ProcessedRule &prevProcessedRule = **iterProcessedRuleColl; 
		
		const WordConsumed *wordConsumed = prevProcessedRule.GetLastWordConsumed();
		assert(wordConsumed);

		const PhraseDictionaryNodeSourceLabel &prevNode = static_cast<const PhraseDictionaryNodeSourceLabel &>(prevProcessedRule.GetLastNode());
		
		//get node for each source LHS
		const LabelList &lhsList = src.GetLabelList(range.GetStartPos(), range.GetEndPos());
		LabelList::const_iterator iterLabelList;
		for (iterLabelList = lhsList.begin(); iterLabelList != lhsList.end(); ++iterLabelList)
		{
			const Word &sourceLHS = *iterLabelList;
			const PhraseDictionaryNodeSourceLabel *node = prevNode.GetChild(sourceLHS, sourceLHS);

			if (node)
			{ // found 1 with matchin source lhs				
				const TargetPhraseCollection *targetPhraseCollection = node->GetTargetPhraseCollection();
				
				if (targetPhraseCollection != NULL)
				{
					ret->Add(*targetPhraseCollection, *wordConsumed, adhereTableLimit, rulesLimit);
				}
			}
		}
		
	}
	
	ret->CreateChartRules(rulesLimit);
	
	return ret;
}

void PhraseDictionaryNewFormat::DeleteDuplicates(ProcessedRuleColl &nodes) const
{
	map<size_t, float> minEntropy;
	map<size_t, float>::iterator iterEntropy;
	
	// find out min entropy for each node id
	ProcessedRuleColl::iterator iter;
	for (iter = nodes.begin(); iter != nodes.end(); ++iter)
	{
		const ProcessedRule *processedRule = *iter;
		const PhraseDictionaryNodeSourceLabel &node = static_cast<const PhraseDictionaryNodeSourceLabel&> (processedRule->GetLastNode());
		size_t nodeId = node.GetId();
		float entropy = node.GetEntropy();
		
		iterEntropy = minEntropy.find(nodeId);
		if (iterEntropy == minEntropy.end())
		{
			minEntropy[nodeId] = entropy;
		}
		else
		{
			float origEntropy = minEntropy[nodeId];
			if (entropy < origEntropy)
			{
				minEntropy[nodeId] = entropy;
			}
		}
	}
	
	// delete nodes which are over min entropy
	size_t ind = 0;
	while (ind < nodes.GetSize())
	{
		const ProcessedRule &processedRule = nodes.Get(ind);
		const PhraseDictionaryNodeSourceLabel &node = static_cast<const PhraseDictionaryNodeSourceLabel&> (processedRule.GetLastNode());
		size_t nodeId = node.GetId();
		float entropy = node.GetEntropy();
		float minEntropy1 = minEntropy[nodeId];
		
		if (entropy > minEntropy1)
		{
			nodes.Delete(ind);
		}
		else
		{
			ind++;
		}
	}
}


