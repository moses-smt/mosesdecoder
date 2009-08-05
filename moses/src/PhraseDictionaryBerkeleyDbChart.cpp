/*
 *  PhraseDictionaryBerkeleyDbChart.cpp
 *  moses
 *
 *  Created by Hieu Hoang on 31/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include "PhraseDictionaryBerkeleyDb.h"
#include "StaticData.h"
#include "DotChartBerkeleyDb.h"
#include "CellCollection.h"
#include "../../BerkeleyPt/src/TargetPhraseCollection.h"

using namespace std;

namespace Moses
{

	
const ChartRuleCollection *PhraseDictionaryBerkeleyDb::GetChartRuleCollection(
																																					InputType const& src
																																					,WordsRange const& range
																																					,bool adhereTableLimit
																																					,const CellCollection &cellColl) const
{
	const StaticData &staticData = StaticData::Instance();
	float weightWP = staticData.GetWeightWordPenalty();
	const LMList &lmList = staticData.GetAllLM();

	// source phrase
	Phrase *cachedSource = new Phrase(src.GetSubString(range));
	m_sourcePhrase.push_back(cachedSource);

	ChartRuleCollection *ret = new ChartRuleCollection();
	m_chartTargetPhraseColl.push_back(ret);

	size_t relEndPos = range.GetEndPos() - range.GetStartPos();
	size_t absEndPos = range.GetEndPos();

	// MAIN LOOP. create list of nodes of target phrases
	ProcessedRuleStackBerkeleyDb &runningNodes = *m_runningNodesVec[range.GetStartPos()];

	const ProcessedRuleStackBerkeleyDb::SavedNodeColl &savedNodeColl = runningNodes.GetSavedNodeColl();
	for (size_t ind = 0; ind < savedNodeColl.size(); ++ind)
	{
		const SavedNodeBerkeleyDb &savedNode = *savedNodeColl[ind];
		const ProcessedRuleBerkeleyDb &prevProcessedRule = savedNode.GetProcessedRule();
		const MosesBerkeleyPt::SourcePhraseNode &prevNode = prevProcessedRule.GetLastNode();
		const WordConsumed *prevWordConsumed = prevProcessedRule.GetLastWordConsumed();
		size_t startPos = (prevWordConsumed == NULL) ? range.GetStartPos() : prevWordConsumed->GetWordsRange().GetEndPos() + 1;

		// search for terminal symbol
		if (startPos == absEndPos)
		{
			const Word &sourceWord = src.GetWord(absEndPos);
			MosesBerkeleyPt::Word *sourceWordBerkeleyDb = m_dbWrapper.ConvertFromMosesSource(m_inputFactorsVec, sourceWord);
	
			if (sourceWordBerkeleyDb != NULL)
			{
				const MosesBerkeleyPt::SourcePhraseNode *node = m_dbWrapper.GetChild(prevNode, *sourceWordBerkeleyDb);
				if (node != NULL)
				{
					// TODO figure out why source word is needed from node, not from sentence
					// prob to do with factors or non-term
					//const Word &sourceWord = node->GetSourceWord();
					WordConsumed *newWordConsumed = new WordConsumed(absEndPos, absEndPos
																														, sourceWord
																														, prevWordConsumed);
					ProcessedRuleBerkeleyDb *processedRule = new ProcessedRuleBerkeleyDb(*node, newWordConsumed);
					runningNodes.Add(relEndPos+1, processedRule);
					
					// cache for cleanup
					m_sourcePhraseNode.push_back(node);
				}

				delete sourceWordBerkeleyDb;
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
			MosesBerkeleyPt::Word *headWordBerkeleyDb = m_dbWrapper.ConvertFromMosesSource(m_inputFactorsVec, headWord);

			if (headWordBerkeleyDb != NULL)
			{
				const MosesBerkeleyPt::SourcePhraseNode *node = m_dbWrapper.GetChild(prevNode, *headWordBerkeleyDb);
				if (node != NULL)
				{
					//const Word &sourceWord = node->GetSourceWord();
					WordConsumed *newWordConsumed = new WordConsumed(startPos, endPos
																														, headWord
																														, prevWordConsumed);

					ProcessedRuleBerkeleyDb *processedRule = new ProcessedRuleBerkeleyDb(*node, newWordConsumed);
					runningNodes.Add(stackInd, processedRule);
				}

				delete headWordBerkeleyDb;
			}
		} // for (iterHeadWords
	}

	// return list of target phrases
	const ProcessedRuleCollBerkeleyDb &nodes = runningNodes.Get(relEndPos + 1);

	size_t rulesLimit = StaticData::Instance().GetRuleLimit();
	ProcessedRuleCollBerkeleyDb::const_iterator iterNode;
	for (iterNode = nodes.begin(); iterNode != nodes.end(); ++iterNode)
	{
		const ProcessedRuleBerkeleyDb &processedRule = **iterNode;
		const MosesBerkeleyPt::SourcePhraseNode &node = processedRule.GetLastNode();
		const WordConsumed *wordConsumed = processedRule.GetLastWordConsumed();
		assert(wordConsumed);

		const MosesBerkeleyPt::TargetPhraseCollection *tpcollBerkeleyDb = m_dbWrapper.GetTargetPhraseCollection(node);

		const TargetPhraseCollection *targetPhraseCollection = m_dbWrapper.ConvertToMoses(
																															*tpcollBerkeleyDb
																															,m_outputFactorsVec
																															,*this
																															,m_weight
																															,weightWP
																															,lmList
																															,*cachedSource);
		delete tpcollBerkeleyDb;
		
		assert(targetPhraseCollection);
		ret->Add(*targetPhraseCollection, *wordConsumed, adhereTableLimit, rulesLimit);
		m_cache.push_back(targetPhraseCollection);

	}
	ret->CreateChartRules(rulesLimit);

	return ret;
}
	
}; // namespace

