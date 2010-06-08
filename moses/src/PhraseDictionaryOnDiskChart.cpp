// $Id: PhraseDictionaryNewFormat.h 3045 2010-04-05 13:07:29Z hieuhoang1972 $
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

#include <algorithm>
#include "PhraseDictionaryOnDisk.h"
#include "StaticData.h"
#include "DotChartOnDisk.h"
#include "CellCollection.h"
#include "../../OnDiskPt/src/TargetPhraseCollection.h"

using namespace std;

namespace Moses
{
	const ChartRuleCollection *PhraseDictionaryOnDisk::GetChartRuleCollection(InputType const& src, WordsRange const& range,
																																						bool adhereTableLimit,const CellCollection &cellColl) const
	{
		const StaticData &staticData = StaticData::Instance();
		float weightWP = staticData.GetWeightWordPenalty();
		const LMList &lmList = staticData.GetAllLM();
		size_t rulesLimit = StaticData::Instance().GetRuleLimit();

		// source phrase
		Phrase *cachedSource = new Phrase(src.GetSubString(range));
		m_sourcePhrase.push_back(cachedSource);
		
		ChartRuleCollection *ret = new ChartRuleCollection();
		m_chartTargetPhraseColl.push_back(ret);
		
		size_t relEndPos = range.GetEndPos() - range.GetStartPos();
		size_t absEndPos = range.GetEndPos();
		
		// MAIN LOOP. create list of nodes of target phrases
		ProcessedRuleStackOnDisk &runningNodes = *m_runningNodesVec[range.GetStartPos()];
		
		// sort save nodes so only do nodes with most counts
		runningNodes.SortSavedNodes();
		size_t numDerivations = 0
		,maxDerivations = 999999; // staticData.GetMaxDerivations();
		bool overThreshold = true;
		
		const ProcessedRuleStackOnDisk::SavedNodeColl &savedNodeColl = runningNodes.GetSavedNodeColl();
		//cerr << "savedNodeColl=" << savedNodeColl.size() << " ";

		for (size_t ind = 0; ind < (savedNodeColl.size()) && ((numDerivations < maxDerivations) || overThreshold) ; ++ind)
		{			
			const SavedNodeOnDisk &savedNode = *savedNodeColl[ind];
			
			const ProcessedRuleOnDisk &prevProcessedRule = savedNode.GetProcessedRule();
			const OnDiskPt::PhraseNode &prevNode = prevProcessedRule.GetLastNode();
			const WordConsumed *prevWordConsumed = prevProcessedRule.GetLastWordConsumed();
			size_t startPos = (prevWordConsumed == NULL) ? range.GetStartPos() : prevWordConsumed->GetWordsRange().GetEndPos() + 1;
			
			// search for terminal symbol
			if (startPos == absEndPos)
			{
				const Word &sourceWord = src.GetWord(absEndPos);
				OnDiskPt::Word *sourceWordBerkeleyDb = m_dbWrapper.ConvertFromMoses(Input, m_inputFactorsVec, sourceWord);
				
				if (sourceWordBerkeleyDb != NULL)
				{
					const OnDiskPt::PhraseNode *node = prevNode.GetChild(*sourceWordBerkeleyDb, m_dbWrapper);
					if (node != NULL)
					{
						// TODO figure out why source word is needed from node, not from sentence
						// prob to do with factors or non-term
						//const Word &sourceWord = node->GetSourceWord();
						WordConsumed *newWordConsumed = new WordConsumed(absEndPos, absEndPos
																														 , sourceWord
																														 , prevWordConsumed);
						ProcessedRuleOnDisk *processedRule = new ProcessedRuleOnDisk(*node, newWordConsumed);
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
			
			size_t nonTermNumWordsCovered = endPos - startPos + 1;
			
			// get target headwords in this span from chart
			const vector<Word> &headWords = cellColl.GetHeadwords(WordsRange(startPos, endPos));
			
			const Word &defaultSourceNonTerm = staticData.GetInputDefaultNonTerminal()
								,&defaultTargetNonTerm = staticData.GetOutputDefaultNonTerminal();

			// go through each SOURCE lhs
			const LabelList &sourceLHSList = src.GetLabelList(startPos, endPos);
			
			LabelList::const_iterator iterSourceLHS;
			for (iterSourceLHS = sourceLHSList.begin(); iterSourceLHS != sourceLHSList.end(); ++iterSourceLHS)
			{
				const Word &sourceLHS = *iterSourceLHS;
				
				OnDiskPt::Word *sourceLHSBerkeleyDb = m_dbWrapper.ConvertFromMoses(Input, m_inputFactorsVec, sourceLHS);
				
				if (sourceLHSBerkeleyDb == NULL)
				{
					delete sourceLHSBerkeleyDb;
					continue; // vocab not in pt. node definately won't be in there
				}
				
				const OnDiskPt::PhraseNode *sourceNode = prevNode.GetChild(*sourceLHSBerkeleyDb, m_dbWrapper);
				delete sourceLHSBerkeleyDb;
				
				if (sourceNode == NULL)
					continue; // didn't find source node
				
				// go through each TARGET lhs
				vector<Word>::const_iterator iterTargetLHS;
				for (iterTargetLHS = headWords.begin(); iterTargetLHS != headWords.end(); ++iterTargetLHS)
				{
					const Word &targetLHS = *iterTargetLHS;

					//cerr << sourceLHS << " " << defaultSourceNonTerm << " " << targetLHS << " " << defaultTargetNonTerm << endl;
					
					//bool isSyntaxNonTerm = (sourceLHS != defaultSourceNonTerm) || (targetLHS != defaultTargetNonTerm);
					bool doSearch = true; //isSyntaxNonTerm ? nonTermNumWordsCovered <=  maxSyntaxSpan :
																//						nonTermNumWordsCovered <= maxDefaultSpan;

					if (doSearch)
					{
						
						OnDiskPt::Word *targetLHSBerkeleyDb = m_dbWrapper.ConvertFromMoses(Output, m_outputFactorsVec, targetLHS);
						
						if (targetLHSBerkeleyDb == NULL)
							continue;
						
						const OnDiskPt::PhraseNode *node = sourceNode->GetChild(*targetLHSBerkeleyDb, m_dbWrapper);
						delete targetLHSBerkeleyDb;
						
						if (node == NULL)
							continue;
						
						// found matching entry
						//const Word &sourceWord = node->GetSourceWord();
						WordConsumed *newWordConsumed = new WordConsumed(startPos, endPos
																														 , targetLHS
																														 , prevWordConsumed);
						
						ProcessedRuleOnDisk *processedRule = new ProcessedRuleOnDisk(*node, newWordConsumed);
						runningNodes.Add(stackInd, processedRule);
						
						m_sourcePhraseNode.push_back(node);
					}					
				} // for (iterHeadWords
				
				delete sourceNode;
				
			} // for (iterLabelListf
			
			// return list of target phrases
			ProcessedRuleCollOnDisk &nodes = runningNodes.Get(relEndPos + 1);
						
			// source LHS
			ProcessedRuleCollOnDisk::const_iterator iterProcessedRuleColl;
			for (iterProcessedRuleColl = nodes.begin(); iterProcessedRuleColl != nodes.end(); ++iterProcessedRuleColl)
			{
				// node of last source word
				const ProcessedRuleOnDisk &prevProcessedRule = **iterProcessedRuleColl; 
				if (prevProcessedRule.Done())
					continue;
				prevProcessedRule.Done(true);
				
				const WordConsumed *wordConsumed = prevProcessedRule.GetLastWordConsumed();
				assert(wordConsumed);
				
				const OnDiskPt::PhraseNode &prevNode = prevProcessedRule.GetLastNode();
				
				//get node for each source LHS
				const LabelList &lhsList = src.GetLabelList(range.GetStartPos(), range.GetEndPos());
				LabelList::const_iterator iterLabelList;
				for (iterLabelList = lhsList.begin(); iterLabelList != lhsList.end(); ++iterLabelList)
				{
					const Word &sourceLHS = *iterLabelList;
					
					OnDiskPt::Word *sourceLHSBerkeleyDb = m_dbWrapper.ConvertFromMoses(Input, m_inputFactorsVec, sourceLHS);
					if (sourceLHSBerkeleyDb == NULL)
						continue;
					
					const TargetPhraseCollection *targetPhraseCollection = NULL;
					const OnDiskPt::PhraseNode *node = prevNode.GetChild(*sourceLHSBerkeleyDb, m_dbWrapper);
					if (node)
					{
						UINT64 tpCollFilePos = node->GetValue();
						std::map<UINT64, const TargetPhraseCollection*>::const_iterator iterCache = m_cache.find(tpCollFilePos);
						if (iterCache == m_cache.end())
						{ // not in case							
							overThreshold = node->GetCount(0) > staticData.GetRuleCountThreshold();
							//cerr << node->GetCount(0) << " ";
							
							const OnDiskPt::TargetPhraseCollection *tpcollBerkeleyDb = node->GetTargetPhraseCollection(GetTableLimit(), m_dbWrapper);
							
							targetPhraseCollection 
									= tpcollBerkeleyDb->ConvertToMoses(m_inputFactorsVec
																								 ,m_outputFactorsVec
																								 ,*this
																								 ,m_weight
																								 ,weightWP
																								 ,lmList
																								 ,*cachedSource
																								 ,m_filePath
																								 , m_dbWrapper.GetVocab());
							
							delete tpcollBerkeleyDb;
							m_cache[tpCollFilePos] = targetPhraseCollection;
						}
						else
						{ // jsut get out of cache
							targetPhraseCollection = iterCache->second;
						}
						
						assert(targetPhraseCollection);
						ret->Add(*targetPhraseCollection, *wordConsumed, adhereTableLimit, rulesLimit);
												
						numDerivations++;					
						
					} // if (node)		
					
					delete node;
					delete sourceLHSBerkeleyDb;
				}	
			}
		} // for (size_t ind = 0; ind < savedNodeColl.size(); ++ind)
		
		ret->CreateChartRules(rulesLimit);

		//cerr << numDerivations << " ";
		
		return ret;
	}
	
}; // namespace

