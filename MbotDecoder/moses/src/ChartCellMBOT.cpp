// $Id: ChartCellMBOT.cpp,v 1.2 2013/01/29 10:18:41 braunefe Exp $
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
#include "ChartCellMBOT.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "ChartHypothesisCollectionMBOT.h"
#include "RuleCubeQueueMBOT.h"
#include "RuleCubeMBOT.h"
#include "WordsRange.h"
#include "Util.h"
#include "StaticData.h"
#include "ChartTranslationOptionMBOT.h"
#include "ChartTranslationOptionList.h"
#include "ChartManager.h"

using namespace std;

namespace Moses
{
extern bool g_debug;

ChartCellMBOT::ChartCellMBOT(size_t startPos, size_t endPos, ChartManager &manager)
  :ChartCell(startPos, endPos, manager)
  ,m_mbotSourceWordLabel(NULL)
  ,m_mbotTargetLabelSet(m_coverage)
{
  //set target label set and coverage
  //WordsRange *mbotCoverage = new WordsRange(startPos,endPos);
  //m_mbotCoverage.push_back(*mbotCoverage);

  //m_mbotTargetLabelSet.AddCoverage(*mbotCoverage);
  //std::cout << "COVERAGE IS " << m_coverage << std::endl;
  m_mbotCoverage.push_back(m_coverage);

  const StaticData &staticData = StaticData::Instance();
  m_nBestIsEnabled = staticData.IsNBestEnabled();
  if (startPos == endPos) {
    const Word &sourceWord = manager.GetSource().GetWord(startPos);

    //make vector containing source word
    //m_mbotSourceWordLabel = new ChartCellLabelMBOT(*mbotCoverage, sourceWord);
    m_mbotSourceWordLabel = new ChartCellLabelMBOT(m_coverage, sourceWord);
    //std::cout << "MBOT COVERAGE" << m_mbotCoverage.front() << std::endl;
    //std::cout << "CCM : Source word : " << sourceWord << " : " << startPos << " : " << endPos << std::endl;
  }
}

ChartCellMBOT::~ChartCellMBOT()
{
  //std::cout << "CHART CELL MBOT DESTROYED" << std::endl;
  delete m_mbotSourceWordLabel;
}

/** Add the given hypothesis to the cell */
bool ChartCellMBOT::AddHypothesis(ChartHypothesisMBOT *hypo)
{
  const std::vector<Word> &targetLHS = hypo->GetTargetLHSMBOT();
  CHECK(targetLHS.size() != 0);

  std::vector<Word>::const_iterator itr_lhs;
  //std::cout << "ADDING HYPOTHESIS AT TARGET LHS (1) : " << std::endl;
  //for(itr_lhs = targetLHS.begin(); itr_lhs != targetLHS.end(); itr_lhs++)
  //{std::cout << "TLHS : " << *itr_lhs << std::endl;}
  //}

  //std::cout << "Collection at Target LHS (1): " << m_mbotHypoColl[targetLHS] << std::endl;
  bool ret = m_mbotHypoColl[targetLHS].AddHypothesis(hypo, m_manager);
  //std::cout << "Collection at Target LHS (2): " << m_mbotHypoColl[targetLHS] << std::endl;
  //return m_mbotHypoColl[targetLHS].AddHypothesis(hypo, m_manager);
  //BEWARE :REDO RIGTH RETURN
  return ret;
}


/** Pruning */
void ChartCellMBOT::PruneToSize()
{
  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    ChartHypothesisCollectionMBOT &coll = iter->second;
    coll.PruneToSize(m_manager);
  }
}


/** Decoding at span level: fill chart cell with mbot hypotheses
 *  (implementation of cube pruning)
 * \param transOptList list of applicable rules to create hypotheses for the cell
 * \param allChartCells entire chart - needed to look up underlying hypotheses
 */
void ChartCellMBOT::ProcessSentence(const ChartTranslationOptionList &transOptList
                                , const ChartCellCollection &allChartCells)
{

  //std::cout << "CCMBOT: PROCESSING SENTENCE IN CHART CELL MBOT" << std::endl;
  const StaticData &staticData = StaticData::Instance();

  // priority queue for applicable rules with selected hypotheses
  //std::cout << "CCMBOT: MAKING RULE CUBE QUEUE" << std::endl;
  RuleCubeQueueMBOT queue(m_manager);

  // add all trans opt into queue. using only 1st child node.
  ChartTranslationOptionList::const_iterator iterList;
  for (iterList = transOptList.begin(); iterList != transOptList.end(); ++iterList)
  {
    //std::cout << "CCMBOT: Iterating through translation options" << std::endl;
    const ChartTranslationOption *transOptOldConst = *iterList;

    //const ChartTranslationOption* ptr_transOptOldConst = &transOptOldConst;
    ChartTranslationOption* transOptOld = const_cast<ChartTranslationOption*>(transOptOldConst);

    //std::cout << "CASTING translation option "<< std::endl;
    ChartTranslationOptionMBOT* transOpt = static_cast<ChartTranslationOptionMBOT*>(transOptOld);

    RuleCubeMBOT *ruleCube = new RuleCubeMBOT(*transOpt, allChartCells, m_manager);
    //std::cout << "RULE CUBE DONE" << std::endl;q
    queue.Add(ruleCube);
    //std::cout << "RULE CUBE ADDED TO QUEUE" << std::endl;
  }
  // pluck things out of queue and add to hypo collection
  const size_t popLimit = staticData.GetCubePruningPopLimit();
  //std::cout << "HYPOTHESES FOR TRANSLATION OPTION : " << std::endl;
  for (size_t numPops = 0; numPops < popLimit && !queue.IsEmptyMBOT(); ++numPops)
  {
    ChartHypothesisMBOT *hypo = queue.PopMBOT();
   //std::cout << "CCM : POPED HYPO : " << (*hypo) << std::endl;
    //std::cout << hypo->GetTranslationOptionMBOT().GetDottedRuleMBOT() << std::endl;
    //std::cout << hypo->GetCurrTargetPhraseMBOT() << std::endl;
    AddHypothesis(hypo);
    //std::cout << "Added Hypothesis..." << std::endl;
  }
  //std::cout << "EXIT PROCESS SENTENCE"<< std::endl;
}


void ChartCellMBOT::ProcessSentenceWithSourceLabels(const ChartTranslationOptionList &transOptList
                        ,const ChartCellCollection &allChartCells, const InputType &source, size_t startPos, size_t endPos)
{

  //std::cout << "CCMBOT: PROCESSING SENTENCE WITH SOURCE LABELS IN CHART CELL MBOT" << std::endl;
  const StaticData &staticData = StaticData::Instance();

  // priority queue for applicable rules with selected hypotheses
  //std::cout << "CCMBOT: MAKING RULE CUBE QUEUE" << std::endl;
  RuleCubeQueueMBOT queue(m_manager);

  // add all trans opt into queue. using only 1st child node.
  ChartTranslationOptionList::const_iterator iterList;
  for (iterList = transOptList.begin(); iterList != transOptList.end(); ++iterList)
   {
       //std::cout << "CCMBOT: Iterating through translation options" << std::endl;
       const ChartTranslationOption *transOptOldConst = *iterList;

       //const ChartTranslationOption* ptr_transOptOldConst = &transOptOldConst;
       ChartTranslationOption* transOptOld = const_cast<ChartTranslationOption*>(transOptOldConst);

       //std::cout << "CASTING translation option "<< std::endl;
       ChartTranslationOptionMBOT* transOpt = static_cast<ChartTranslationOptionMBOT*>(transOptOld);

       std::vector<Word> cellSourceLabels;

       //std::cerr << "NON TERMINALS FOR THIS CELL : " << std::endl;
       NonTerminalSet::const_iterator iter;
       for(iter = source.GetLabelSet(startPos,endPos).begin();iter != source.GetLabelSet(startPos,endPos).end();iter++)
       {
           const Word &word = *iter;
           //std::cerr << "[" << startPos <<"," << endPos << "]="<< word << "(" << word.IsNonTerminal() << ") ";
           cellSourceLabels.push_back(*iter);

        }

       //CHECK IF SOURCE LABEL STORED IN DOTTED RULE DOES MATCH SOURCE LABEL IN DOTTED RULE
       //Loop over source labels

       bool isMatch = false; //if no target phrase in the translation option matches the source label then do not create cube for it
       std::vector<Word>::iterator itrLab;

       //Only create cubes for translation options that have a target phrase that matches label in the source side parse tree
       vector<TargetPhrase*> :: const_iterator itr_coll;
       for(itrLab = cellSourceLabels.begin(); itrLab != cellSourceLabels.end(); itrLab++)
       {
    	   //if the tranlsation option has the correct source label (via dotted rule)
    	   for(itr_coll = transOpt->GetTargetPhraseCollection().GetCollection().begin(); itr_coll != transOpt->GetTargetPhraseCollection().GetCollection().end(); itr_coll++)
    	    {
    	           TargetPhraseMBOT * mbotTp = static_cast<TargetPhraseMBOT*>(*itr_coll);
    	           //std::cerr << "CHECKED TARGET PHRASE ... : " << *mbotTp << std::endl;

    	           //indicate that the rule <s> X <\s> matches each source label
    	           if(mbotTp->GetMBOTPhrase(0)->GetWord(0).GetFactor(0)->GetString().compare("<s>") == 0)
    	           {
    	        	  //std::cerr << "WE ARE LOOKING FOR : " << mbotTp->GetMBOTPhrase(0)->GetWord(0).GetFactor(0)->GetString() << std::endl;
    	        	  //std::cerr << "WE COMPARE IT TO : " << "<s>" << std::endl;
    	              mbotTp->setMatchesSource(true);
    	           }

    	           std::vector<Word>::iterator itrLab;
    	           for(itrLab = cellSourceLabels.begin(); itrLab != cellSourceLabels.end(); itrLab++)
    	           {
    	        	   //std::cerr << "COMPARING : " << mbotTp->GetSourceLHS() << " with " << *itrLab << std::endl;
    	        	   //std::cerr << "WHAT IS THE VALUE OF THE OPTION : "<< StaticData::Instance().IsMatchingSourceAtRuleApplication() << std::endl;

    	        	   //check we want to match source labels for each chart cell
    	        	   if(StaticData::Instance().IsMatchingSourceAtRuleApplication() == 1)
    	        	   {

    	        		   if(mbotTp->GetSourceLHS() == *itrLab)
    	        		   {
    	        			   isMatch = true;
    	        			   //mark target phrases that match the source label
    	        			   //=> at hypothesis creation, we will only consider target phrase that match the source label
    	        			   mbotTp->setMatchesSource(true);
    	        			   //std::cerr << "LABEL TO MATCH : " << *itrLab << std::endl;
    	        			   //std::cerr << "TARGET PHRASE MARKED..."<< *mbotTp << std::endl;
    	        		   }
    	        	   }
    	        	   else
    	        	   {
    	        		   	isMatch = true;
    	        		   	mbotTp->setMatchesSource(true);
    	        	   }
    	           }
    	    }
       }
       if(isMatch == true)
       {
    	   /*std::cerr << "------------------------------------------"<< std::endl;
    	   std::cerr << "CREATING CUBE FOR TRANSLATION OPTION : " << std::endl;
    	   std::cerr << *transOpt  << std::endl;
    	   std::cerr << "------------------------------------------"<< std::endl;*/
    	   RuleCubeMBOT *ruleCube = new RuleCubeMBOT(*transOpt, allChartCells, m_manager);
    	   //std::cerr << "CUBE CREATED : " << std::endl;
    	   queue.Add(ruleCube);
       }
     }

  // pluck things out of queue and add to hypo collection
  const size_t popLimit = staticData.GetCubePruningPopLimit();
  //std::cout << "HYPOTHESES FOR TRANSLATION OPTION : " << std::endl;
  size_t numPops = 0;
  while(numPops < popLimit && !queue.IsEmptyMBOT())
  {
    ChartHypothesisMBOT *hypo = queue.PopMBOT();
    //if we find, on the translation dimension, a target phrase that does not
    //match the source label, then we return a NULL hypothesis
    if(hypo == NULL)
    {
    	//Do not count null hypos : increase pop limit for each such hypos
    	//std::cerr << "NULL HYPO : NUMBER OF POPS NOT INCREASED  " << numPops << std::endl;
    }
    else
    {
    	//otherwise, we add it to the chart cell
    	//std::cerr << "------------------------------------------"<< std::endl;
    	//std::cerr << "POPPING HYPOTHESIS FROM CUBE : " << std::endl;

    	//std::cerr << hypo->GetTranslationOptionMBOT() << std::endl;
    	//std::cerr << (*hypo) << std::endl;
    	//std::cerr << hypo->GetCurrTargetPhraseMBOT() << std::endl;
    	AddHypothesis(hypo);
    	numPops++;
       //std::cout << "Added Hypothesis..." << std::endl;
    }
  }
  //std::cout << "EXIT PROCESS SENTENCE"<< std::endl;
}

void ChartCellMBOT::SortHypotheses()
{
   //std::cout << "IN CHART CELL MBOT : SORTING HYPOS : " << std::endl;
  // sort each mini cells & fill up target lhs list
  CHECK(m_mbotTargetLabelSet.EmptyMBOT());
  std::map< std::vector<Word>, ChartHypothesisCollectionMBOT>::iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    ChartHypothesisCollectionMBOT &coll = iter->second;
    //std::cout << "CONSIDERED COLLECTION : " << coll << std::endl;
    //std::cout << "ADDING CONSTITUENT TO TARGET LABEL SET : " << std::endl;
    //std::cout << "SIZE OF VECTOR " << (iter->first).size() << std::endl;
    //std::vector<Word>::iterator itr_words;
    //std::vector<Word> target = iter->first;
    //for(itr_words = target.begin(); itr_words != target.end(); itr_words++)
    //{
      //  std::cout << "CONSTITUENT TO ADD : " << *itr_words << std::endl;
    //}
    //std::cout << "SIZE OF VECTOR " << (iter->first).size() << std::endl;
    m_mbotTargetLabelSet.AddConstituent(iter->first, coll);
    //std::cout << "SORTING HYPOTHESES : " << coll << std::endl;
    coll.SortHypotheses();
  }
}

/** Return the highest scoring hypothesis in the cell */
const ChartHypothesisMBOT *ChartCellMBOT::GetBestHypothesis() const
{
  //std:cout << "Getting Best Hypo : " << std::endl;
  const ChartHypothesisMBOT *ret = NULL;
  float bestScore = -std::numeric_limits<float>::infinity();
  //std::cout << "BestScore 1: " << bestScore << std::endl;

  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::const_iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    const HypoListMBOT &sortedList = iter->second.GetSortedHypothesesMBOT();
    CHECK(sortedList.size() > 0);

    const ChartHypothesisMBOT *hypo = sortedList[0];
    if (hypo->GetTotalScore() > bestScore) {
      bestScore = hypo->GetTotalScore();
      //std::cout << "BestScore 2: " << bestScore << std::endl;
      ret = hypo;
    };
  }

  return ret;
}

void ChartCellMBOT::CleanupArcList()
{
  //std::cout << "Cleaning Up Arc List MBOT" << std::endl;
  // only necessary if n-best calculations are enabled
  if (!m_nBestIsEnabled) return;

  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
        ChartHypothesisCollectionMBOT &coll = iter->second;
        coll.CleanupArcList();
  }
}

void ChartCellMBOT::OutputSizes(std::ostream &out) const
{
  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::const_iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    const std::vector<Word> &targetLHS = iter->first;
    const ChartHypothesisCollectionMBOT &coll = iter->second;

    //iterate over words of rhs and given size of hypothesis collection
    std::vector<Word> :: const_iterator itr_target_lhs;
    int counter = 1;
    for(itr_target_lhs = targetLHS.begin(); itr_target_lhs != targetLHS.end(); itr_target_lhs++)
    {
            Word oneTargetLHS = *itr_target_lhs;
            out << oneTargetLHS << "(" << counter << ")" << " ";
            counter++;
    }
    out << "=" << coll.GetSize() << " ";
  }
}

size_t ChartCellMBOT::GetSize() const
{
  size_t ret = 0;
  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::const_iterator iter;
  for (iter = m_mbotHypoColl.begin(); iter != m_mbotHypoColl.end(); ++iter) {
    const ChartHypothesisCollectionMBOT &coll = iter->second;
    ret += coll.GetSizeMBOT();
  }

  return ret;
}

void ChartCellMBOT::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned, bool> &reachable) const
{
  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>:: const_iterator iterOutside;
  for (iterOutside = m_mbotHypoColl.begin(); iterOutside != m_mbotHypoColl.end(); ++iterOutside) {
    const ChartHypothesisCollectionMBOT &coll = iterOutside->second;
    coll.GetSearchGraph(translationId, outputSearchGraphStream, reachable);
  }
}

std::ostream& operator<<(std::ostream &out, const ChartCellMBOT &cell)
{

  std::cout << "------------------------------------"<< std::endl;
  std::cout << "THIS IS AN MBOT CHART CELL" << std::endl;
  std::cout << "------------------------------------"<< std::endl;

  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT>::const_iterator iterOutside;
  for (iterOutside = cell.m_mbotHypoColl.begin(); iterOutside != cell.m_mbotHypoColl.end(); ++iterOutside) {
    const std::vector<Word> &targetLHS = iterOutside->first;
    std::vector<Word> :: const_iterator itr_words;
    int counter = 1;

    for(itr_words = targetLHS.begin(); itr_words != targetLHS.end(); itr_words++)
    {
        Word oneTargetLHS = *itr_words;
        cerr << oneTargetLHS << "(" << counter << ")" << " :" ;
    }

    out << endl;

    const ChartHypothesisCollectionMBOT &coll = iterOutside->second;
    //new : inserted for testing
    std::cout << "Displaying chart hypothesis collection" << std::endl;
    cerr << coll;
  }

  /*
  ChartCell::HCType::const_iterator iter;
  for (iter = cell.m_hypos.begin(); iter != cell.m_hypos.end(); ++iter)
  {
  	const ChartHypothesis &hypo = **iter;
  	out << hypo << endl;
  }
   */

  return out;
}

} // namespace
