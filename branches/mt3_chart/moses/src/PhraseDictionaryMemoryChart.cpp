
#include "PhraseDictionaryMemory.h"
#include "FactorCollection.h"
#include "InputType.h"
#include "ChartRuleCollection.h"
#include "CellCollection.h"
#include "DotChart.h"
#include "StaticData.h"

using namespace std;
using namespace Moses;

const ChartRuleCollection *PhraseDictionaryMemory::GetChartRuleCollection(
																	InputType const& src
																	,WordsRange const& range
																	,bool adhereTableLimit
																	,const CellCollection &cellColl) const
{
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
		const PhraseDictionaryNode &prevNode = prevProcessedRule.GetLastNode();
		const WordConsumed *prevWordConsumed = prevProcessedRule.GetLastWordConsumed();
		size_t startPos = (prevWordConsumed == NULL) ? range.GetStartPos() : prevWordConsumed->GetWordsRange().GetEndPos() + 1;

		// search for terminal symbol
		if (startPos == absEndPos)
		{
			const Word &sourceWord = src.GetWord(absEndPos);
			const PhraseDictionaryNode *node = static_cast<const PhraseDictionaryNodeMemory&>(prevNode).GetChild(sourceWord);
			if (node != NULL)
			{
				const Word &sourceWord = node->GetSourceWord();
				WordConsumed *newWordConsumed = new WordConsumed(absEndPos, absEndPos
																													, sourceWord
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

		// get headwords in this span from chart
		const vector<Word> &headWords = cellColl.GetHeadwords(WordsRange(startPos, endPos));
		vector<Word>::const_iterator iterHeadWords;

		// go thru each headword & see if in phrase table
		for (iterHeadWords = headWords.begin(); iterHeadWords != headWords.end(); ++iterHeadWords)
		{
			const Word &headWord = *iterHeadWords;
			const PhraseDictionaryNode *node = static_cast<const PhraseDictionaryNodeMemory&>(prevNode).GetChild(headWord);
			if (node != NULL)
			{
				//const Word &sourceWord = node->GetSourceWord();
				WordConsumed *newWordConsumed = new WordConsumed(startPos, endPos
																													, headWord
																													, prevWordConsumed);

				ProcessedRule *processedRule = new ProcessedRule(*node, newWordConsumed);
				runningNodes.Add(stackInd, processedRule);
			}
		} // for (iterHeadWords
	}

	// return list of target phrases
	const ProcessedRuleColl &nodes = runningNodes.Get(relEndPos + 1);

	size_t rulesLimit = StaticData::Instance().GetRuleLimit();
	ProcessedRuleColl::const_iterator iterNode;
	for (iterNode = nodes.begin(); iterNode != nodes.end(); ++iterNode)
	{
		const ProcessedRule &processedRule = **iterNode;
		const PhraseDictionaryNode &node = processedRule.GetLastNode();
		const WordConsumed *wordConsumed = processedRule.GetLastWordConsumed();
		assert(wordConsumed);

		const TargetPhraseCollection *targetPhraseCollection = node.GetTargetPhraseCollection();

		if (targetPhraseCollection != NULL)
		{
			ret->Add(*targetPhraseCollection, *wordConsumed, adhereTableLimit, rulesLimit);
		}
	}
	ret->CreateChartRules(rulesLimit);

	return ret;
}
