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

#include "ChartRuleLookupManagerOnDisk.h"

#include <algorithm>

#include "PhraseDictionaryOnDisk.h"
#include "StaticData.h"
#include "DotChartOnDisk.h"
#include "CellCollection.h"
#include "ChartTranslationOptionList.h"
#include "../../OnDiskPt/src/TargetPhraseCollection.h"

using namespace std;

namespace Moses
{

ChartRuleLookupManagerOnDisk::ChartRuleLookupManagerOnDisk(
    const InputType &sentence,
    const CellCollection &cellColl,
    const PhraseDictionaryOnDisk &dictionary,
    OnDiskPt::OnDiskWrapper &dbWrapper,
    const LMList *languageModels,
    const WordPenaltyProducer *wpProducer,
    const std::vector<FactorType> &inputFactorsVec,
    const std::vector<FactorType> &outputFactorsVec,
    const std::vector<float> &weight,
    const std::string &filePath)
  : ChartRuleLookupManager(sentence, cellColl)
  , m_dictionary(dictionary)
  , m_dbWrapper(dbWrapper)
  , m_languageModels(languageModels)
  , m_wpProducer(wpProducer)
  , m_inputFactorsVec(inputFactorsVec)
  , m_outputFactorsVec(outputFactorsVec)
  , m_weight(weight)
  , m_filePath(filePath)
{
    assert(m_runningNodesVec.size() == 0);
    size_t sourceSize = sentence.GetSize();
    m_runningNodesVec.resize(sourceSize);

    for (size_t ind = 0; ind < m_runningNodesVec.size(); ++ind)
    {
        ProcessedRuleOnDisk *initProcessedRule = new ProcessedRuleOnDisk(m_dbWrapper.GetRootSourceNode());

        ProcessedRuleStackOnDisk *processedStack = new ProcessedRuleStackOnDisk(sourceSize - ind + 1);
        processedStack->Add(0, initProcessedRule); // init rule. stores the top node in tree

        m_runningNodesVec[ind] = processedStack;
    }
}

ChartRuleLookupManagerOnDisk::~ChartRuleLookupManagerOnDisk()
{
    std::map<UINT64, const TargetPhraseCollection*>::const_iterator iterCache;
    for (iterCache = m_cache.begin(); iterCache != m_cache.end(); ++iterCache)
    {
        delete iterCache->second;
    }
    m_cache.clear();

    RemoveAllInColl(m_runningNodesVec);
    RemoveAllInColl(m_sourcePhraseNode);
}

void ChartRuleLookupManagerOnDisk::GetChartRuleCollection(
    const WordsRange &range,
    bool adhereTableLimit,
    ChartTranslationOptionList &outColl)
{
    const StaticData &staticData = StaticData::Instance();
    size_t rulesLimit = StaticData::Instance().GetRuleLimit();
    
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
            const Word &sourceWord = GetSentence().GetWord(absEndPos);
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
        const NonTerminalSet &headWords = GetCellCollection().GetHeadwords(WordsRange(startPos, endPos));
        
        const Word &defaultSourceNonTerm = staticData.GetInputDefaultNonTerminal()
                            ,&defaultTargetNonTerm = staticData.GetOutputDefaultNonTerminal();

        // go through each SOURCE lhs
        const NonTerminalSet &sourceLHSSet = GetSentence().GetLabelSet(startPos, endPos);
        
        NonTerminalSet::const_iterator iterSourceLHS;
        for (iterSourceLHS = sourceLHSSet.begin(); iterSourceLHS != sourceLHSSet.end(); ++iterSourceLHS)
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
            NonTerminalSet::const_iterator iterTargetLHS;
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
            const NonTerminalSet &lhsSet = GetSentence().GetLabelSet(range.GetStartPos(), range.GetEndPos());
            NonTerminalSet::const_iterator iterLabelSet;
            for (iterLabelSet = lhsSet.begin(); iterLabelSet != lhsSet.end(); ++iterLabelSet)
            {
                const Word &sourceLHS = *iterLabelSet;
                
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
                        
                        const OnDiskPt::TargetPhraseCollection *tpcollBerkeleyDb = node->GetTargetPhraseCollection(m_dictionary.GetTableLimit(), m_dbWrapper);
                        
                        targetPhraseCollection 
                                = tpcollBerkeleyDb->ConvertToMoses(m_inputFactorsVec
                                                                                             ,m_outputFactorsVec
                                                                                             ,m_dictionary
                                                                                             ,m_weight
                                                                                             ,m_wpProducer
                                                                                             ,*m_languageModels
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
                    outColl.Add(*targetPhraseCollection, *wordConsumed, adhereTableLimit, rulesLimit);
                                            
                    numDerivations++;					
                    
                } // if (node)		
                
                delete node;
                delete sourceLHSBerkeleyDb;
            }	
        }
    } // for (size_t ind = 0; ind < savedNodeColl.size(); ++ind)
    
    outColl.CreateChartRules(rulesLimit);

    //cerr << numDerivations << " ";		
}
	
} // namespace Moses
