// $Id: ChartHypothesisMBOT.cpp,v 1.3 2013/01/15 14:10:50 braunefe Exp $
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
#include <vector>
#include <set>
#include <list>
#include "ChartHypothesisMBOT.h"
#include "ChartHypothesis.h"
#include "RuleCubeItemMBOT.h"
#include "ChartCell.h"
#include "ChartManager.h"
#include "TargetPhraseMBOT.h"
#include "Phrase.h"
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "LMList.h"
//#include "ChartTranslationOptionMBOT.h"
//#include "RuleCubeItemMBOT.h"
#include "FFState.h"

namespace Moses
{

#ifdef USE_HYPO_POOL
ObjectPool<ChartHypothesisMBOT> ChartHypothesisMBOT::s_objectPool("ChartHypothesisMBOT", 300000);
#endif

/** Create a hypothesis from a rule */
ChartHypothesisMBOT::ChartHypothesisMBOT(const ChartTranslationOptions &transOpt,
                                 const RuleCubeItemMBOT &mbotItem,
                                 ChartManager &manager)
  :ChartHypothesis(transOpt, mbotItem, manager)
  ,m_mbotTargetPhrase(const_cast <TargetPhraseMBOT*> (
                                        mbotItem.GetTranslationDimensionMBOT().GetTargetPhraseMBOT()))
  //,m_mbotArcList(NULL)
  //,m_mbotWinningHypo(NULL)
{
  // underlying hypotheses for sub-spans
  const std::vector<HypothesisDimension> &childEntries = mbotItem.GetHypothesisDimensionsMBOT();
  m_prevHypos.reserve(childEntries.size());
  std::vector<HypothesisDimension>::const_iterator iter;
  for (iter = childEntries.begin(); iter != childEntries.end(); ++iter)
  {
    m_prevHypos.push_back(iter->GetHypothesis());
  }
}


ChartHypothesisMBOT::~ChartHypothesisMBOT()
{
	//Fabienne Braune : nothing to delete here, everything done in base class
    //delete m_arcList;
}

/** Create full output phrase that is contained in the hypothesis (and its children)
 * \param outPhrase full output phrase
 */

//! has to create a possibly discontinuous output phrase
void ChartHypothesisMBOT::CreateOutputPhrase(Phrase &outPhrase, ProcessedNonTerminals & processedNonTerminals) const
{
  // add word as parameter
   //std::cout << "AT BEGINNING OF METHOD : ADDED HYPOTHESIS : " << (*this) << std::endl;
   processedNonTerminals.AddHypothesis(this);
   processedNonTerminals.AddStatus(this->GetId(),1);

   //std::cout << "NUMBER OF RECURSION " << processedNonTerminals.GetRecNumber() << std::endl;
   const ChartHypothesisMBOT * currentHypo = processedNonTerminals.GetHypothesis();
   //std::cout << "CURRENT HYPOTHESIS : " << currentHypo << std::endl;
   //const TargetPhraseMBOT currentTarget = currentHypo->GetCurrTargetPhraseMBOT();
   //std::cout << "CREATING TARGET PHRASE : " << currentTarget << std::endl;

   //look at size : if there is only one m_mbot phrase, process as usual
   //if there are several mbot phrases : split hypotheses

   //std::vector<Phrase> targetPhrases = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases();
   const std::vector<const AlignmentInfoMBOT*> *alignedTargets = currentHypo->GetCurrTargetPhraseMBOT()->GetMBOTAlignments();
   //std::vector<Word> targetLHS = currentHypo->GetCurrTargetPhraseMBOT().GetTargetLHSMBOT();

   //while(GetProcessingPhrase()->GetStatus() < targetPhrases.size())
   //{
        int currentlyProcessed = processedNonTerminals.GetStatus(currentHypo->GetId()) -1;
        //std::cout << "CURRENTLY PROCESSED : " << currentlyProcessed << std::endl;

        //GetProcessingPhrase()->IncrementStatus();
        //std::cout << "CURENT STATUS : " << GetProcessingPhrase()->GetStatus() << std::endl;

        CHECK(currentHypo->GetCurrTargetPhraseMBOT()->GetMBOTPhrases().GetSize() > currentlyProcessed);
        size_t mbotSize = currentHypo->GetCurrTargetPhraseMBOT()->GetMBOTPhrases().GetSize();
        //std::cout << "Getting current Phrase : " << GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() << std::endl;

        //if several mbot phrases, look at status
         const Phrase * currentPhrase = currentHypo->GetCurrTargetPhraseMBOT()->GetMBOTPhrase(currentlyProcessed);
         //std::cout << "Current MBOT Phrase : " << currentPhrase << std::endl;
        //std::vector<Phrase>::iterator itr_mbot_phrases;

        //if(targetPhrases.size() == 1)
        //{

         //if several mbot phrases, look at status
        //if(mbotSize > 1)
        //{
           int position = 0;
        //Phrase currentPhrase = targetPhrases.front();
            //std::cout << "CURRENT PHRASE : " << currentPhrase << std::endl;
            for (size_t pos = 0; pos < currentPhrase->GetSize(); ++pos) {

                //std::cout << "POSITION : " << pos << std::endl;
                const Word &word = currentPhrase->GetWord(pos);
                //std::cout << "CURRENT WORD : " << word << std::endl;
                if (word.IsNonTerminal()) {
                    const AlignmentInfoMBOT::NonTermIndexMapPointer nonTermIndexMap =
                    alignedTargets->at(currentlyProcessed)->GetNonTermIndexMap();

                    int sizeOfMap = nonTermIndexMap->size();
                    //std::cout << "MBOT current map : " << sizeOfMap << std::endl;
                // non-term. fill out with prev hypo
                //get hypo corresponding to mbot phrase
                size_t nonTermInd = nonTermIndexMap->at(pos);
                //std::cout << "NON TERM IND : " << nonTermInd << std::endl;
                processedNonTerminals.IncrementRec();
                //std::cout << "TAKING PREVIOUS HYPO of " << *currentHypo << std::endl;
                const ChartHypothesisMBOT *prevHypo = static_cast<const ChartHypothesisMBOT*> (currentHypo->GetPrevHypo(nonTermInd));
                //MBOT CONDITION HERE ::::

                //std::cout << "PREVIOUS HYPO" << *prevHypo << std::endl;
                //Check if hypothesis has already been used
                /*while(processedNonTerminals.FindRange(prevHypo->GetCurrSourceRange()))
                {
                    std::cout << "ALREADY PROCESSED TAKE NEXT"<< std::endl;
                    nonTermInd++;
                    prevHypo = currentHypo->GetPrevHypoMBOT(nonTermInd);
                }
                //if hypo is not found then look at first associated
                std::cout << "BEFORE NULL : CURRENT HYPO IS : " << *currentHypo << std::endl;
                std::cout << "THIS HYPOTHESIS : " << *prevHypo << std::endl;
                //prevHypo->GetProcessingPhrase()->ResetStatus();
                */
                prevHypo->CreateOutputPhrase(outPhrase,processedNonTerminals);
                }
                else {
                //std::cout << "ADDED WORD :" << word << std::endl;
                outPhrase.AddWord(word);
                //std::cout << "MBOT SIZE : " << mbotSize << " : Status : " << processedNonTerminals.GetStatus(currentHypo->GetId()) << std::endl;
                //std::cout << "CURRENT POSITION : " << pos << std::endl;
                //Add processed leaves to processed span unless leaf is mbot target phrase
                }
                //std::cout << "MbotSize and status " << mbotSize << " : " << processedNonTerminals.GetStatus(currentHypo->GetId()) << std::endl;
                 if(//(currentHypo->GetCurrSourceRange().GetStartPos() == currentHypo->GetCurrSourceRange().GetEndPos())
                    //&& (
                        mbotSize > (processedNonTerminals.GetStatus(currentHypo->GetId()))
                          //)
                        && (pos == currentPhrase->GetSize() - 1) )
                        {
                            processedNonTerminals.IncrementStatus(currentHypo->GetId());
                            }
                /*if(
                   (pos == currentPhrase.GetSize() - 1)
                   //(currentHypo->GetCurrSourceRange().GetStartPos() == currentHypo->GetCurrSourceRange().GetEndPos())
                   && ( mbotSize == (currentHypo->GetProcessingPhrase()->GetStatus() ) || mbotSize == 1))
                   //&& (currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() == 1))
                   processedNonTerminals.AddRangeMergeAll(currentHypo->GetCurrSourceRange());
                }*/
        }
        //std::cout << "GOES OUT OF RECURSION" << std::endl;
        //std::cout << "CURRENT HYPOTHESIS : " << currentHypo << std::endl;
        //mark this hypothesis as done.
        processedNonTerminals.DecrementRec();
    //}
//std::cout << "THIS HYPOTHESIS : " << *prevHypo << std::endl;
}

/** check, if two hypothesis can be recombined.
    this is actually a sorting function that allows us to
    keep an ordered list of hypotheses. This makes recombination
    much quicker.
*/
int ChartHypothesisMBOT::RecombineCompare(const ChartHypothesisMBOT &compare) const
{
    //std::cout << "In recombine compare : " << std::endl;
	int comp = 0;
  // -1 = this < compare
  // +1 = this > compare
  // 0	= this ==compare

  for (unsigned i = 0; i < m_ffStates.size(); ++i)
	{
    if (m_ffStates[i] == NULL || compare.m_ffStates[i] == NULL)
      comp = m_ffStates[i] - compare.m_ffStates[i];
		else
      comp = m_ffStates[i]->Compare(*compare.m_ffStates[i]);

		if (comp != 0)
			return comp;
  }
  //std::cout << "Out of recombine compare " << std::endl;
  return 0;
}

//do not overwrite, somehing nasty is going on in there
void ChartHypothesisMBOT::CalcScoreMBOT()
{
  //std::cout << "CHART HYPOHTESIS : COMPUTING SCORE" << std::endl;
  // total scores from prev hypos
  std::vector<const ChartHypothesis*>::iterator iter;
  for (iter = m_prevHypos.begin(); iter != m_prevHypos.end(); ++iter) {
    const ChartHypothesisMBOT* prevHypo = static_cast<const ChartHypothesisMBOT*> (*iter);
    const ScoreComponentCollection &scoreBreakdown = prevHypo->GetScoreBreakdown();
    m_scoreBreakdown.PlusEquals(scoreBreakdown);
  }

  // translation models & word penalty
  const ScoreComponentCollection &scoreBreakdown = GetCurrTargetPhraseMBOT()->GetScoreBreakdown();

  m_scoreBreakdown.PlusEquals(scoreBreakdown);

	// compute values of stateless feature functions that were not
  // cached in the translation option-- there is no principled distinction

  //const vector<const StatelessFeatureFunction*>& sfs =
  //  m_manager.GetTranslationSystem()->GetStatelessFeatureFunctions();
	// TODO!
  //for (unsigned i = 0; i < sfs.size(); ++i) {
  //  sfs[i]->ChartEvaluate(m_targetPhrase, &m_scoreBreakdown);
  //}

  const std::vector<const StatefulFeatureFunction*>& ffs =
    m_manager.GetTranslationSystem()->GetStatefulFeatureFunctions();

  //std::cerr << "Computing score for hypothesis : " << *this << std::endl;
  for (unsigned i = 0; i < ffs.size(); ++i) {
        //std::cout << "Evaluating MBOT" << std::endl;
		m_ffStates[i] = ffs[i]->EvaluateMBOT(*this,i,&m_scoreBreakdown);
  }

  m_totalScore	= m_scoreBreakdown.GetWeightedScore();
  //std::cerr << "Computed score for hypothesis : " << *this << std::endl;
  //std::cerr << "TOTAL SCORE COMPUTED FOR HYPOTHESIS : " << m_totalScore << std::endl;
}

/*void ChartHypothesisMBOT::AddArc(ChartHypothesisMBOT *loserHypo)
{
  //std::cout << "Adding arc" << std::endl;
  //std::cout << "Looser Hypo . " << (*loserHypo) << std::endl;
  if (!m_mbotArcList) {
    if (loserHypo->m_mbotArcList) {
       // std::cout << "Getting Arc" << std::endl; // we don't have an arcList, but loser does
      this->m_mbotArcList = loserHypo->m_mbotArcList;  // take ownership, we'll delete
      loserHypo->m_mbotArcList = 0;                // prevent a double deletion
    } else {
       // std::cout << "Getting Arc 2" << std::endl;
      this->m_mbotArcList = new ChartArcListMBOT();
      // std::cout << "New arc here" << std::endl;

    }
  } else {
      // std::cout << "Getting Arc 3" << std::endl;
    if (loserHypo->m_mbotArcList) {  // both have an arc list: merge. delete loser
      // std::cout << "Getting Arc 4" << std::endl;
      size_t my_size = m_mbotArcList->size();
      //std::cout << "Size of ArcList : " << m_mbotArcList->size() << std::endl;
      size_t add_size = loserHypo->m_mbotArcList->size();
      //std::cout << "Size of looser ArcList : " << m_mbotArcList->size() << std::endl;
      this->m_mbotArcList->resize(my_size + add_size, 0);
      std::memcpy(&(*m_mbotArcList)[0] + my_size, &(*loserHypo->m_mbotArcList)[0], add_size * sizeof(ChartHypothesisMBOT *));
      delete loserHypo->m_mbotArcList;
      loserHypo->m_mbotArcList = 0;
    } else { // loserHypo doesn't have any arcs
      // DO NOTHING
    }
  }
  //std::cout << "Looser hypo pushed back" << std::endl;
  m_mbotArcList->push_back(loserHypo);
  //std::cout << "Going out of method" << std::endl;
}*/

struct CompareChartChartHypothesisMBOTTotalScore {
  bool operator()(const ChartHypothesis* hypo1, const ChartHypothesis* hypo2) const {
    return hypo1->GetTotalScore() > hypo2->GetTotalScore();
  }
};

//Fabienne Braune : should be handled by base class
/*
void ChartHypothesisMBOT::CleanupArcList()
{

  //std::cout << "CLEANING UP ARC LIST IN HYPO MBOT" << std::endl;
  // point this hypo's main hypo to itself
  m_winningHypo = this;

  //std::cout << "Winning Hypo : " << (*m_mbotWinningHypo) << std::endl;

  if (!m_mbotArcList)
  {
        //std::cout << "No mbot arc list" << std::endl;
        return;
  }

  const StaticData &staticData = StaticData::Instance();
  //std::cout << "Static data instance here" << std::endl;
  size_t nBestSize = staticData.GetNBestSize();
  //std::cout << "Nbest size here : " << nBestSize<< std::endl;
  bool distinctNBest = staticData.GetDistinctNBest() || staticData.UseMBR() || staticData.GetOutputSearchGraph();
  //std::cout << "Nbest size here 2" << distinctNBest << std::endl;

  if (!distinctNBest && m_mbotArcList->size() > nBestSize) {
    //std::cout << "Arc List bigger : " << m_mbotArcList->size() << std::endl;
    // prune arc list only if there too many arcs
    nth_element(m_mbotArcList->begin()
                , m_mbotArcList->begin() + nBestSize - 1
                , m_mbotArcList->end()
                , CompareChartChartHypothesisMBOTTotalScore());
    //std::cout << "List sorted" << std::endl;

    // delete bad ones
    ChartArcListMBOT::iterator iter;
    for (iter = m_mbotArcList->begin() + nBestSize ; iter != m_mbotArcList->end() ; ++iter) {
      //std::cout << "Trying to get arcs" << std::endl;
      ChartHypothesisMBOT *arc = *iter;
      //std::cout << "Before deleting arcs" << std::endl;
      ChartHypothesisMBOT::DeleteMBOT(arc);
    }
    m_mbotArcList->erase(m_mbotArcList->begin() + nBestSize
                     , m_mbotArcList->end());
  }

  // set all arc's main hypo variable to this hypo
  ChartArcListMBOT::iterator iter = m_mbotArcList->begin();
  for (; iter != m_mbotArcList->end() ; ++iter) {
    //std::cout << "Trying to get arc" << std::endl;
    ChartHypothesisMBOT *arc = *iter;
    arc->SetWinningHypo(this);
  }

  //cerr << m_arcList->size() << " ";
}*/

//void ChartHypothesisMBOT::SetWinningHypo(const ChartHypothesisMBOT *hypo)
//{
//  m_mbotWinningHypo = hypo;
//}

//TO_STRING_BODY(ChartHypothesisMBOT)

// friend
std::ostream& operator<<(std::ostream& out, const ChartHypothesisMBOT& hypo)
{

  out << hypo.GetId();

	// recombination
	if (hypo.GetWinningHypothesis() != NULL &&
			hypo.GetWinningHypothesis() != &hypo)
	{
		out << "->" << hypo.GetWinningHypothesis()->GetId();
	}

    //out << " " << hypo.GetCurrTargetPhraseMBOT();
	//out << " " << hypo.GetTranslationOptionMBOT();
      //<< " " << outPhrase
    out  << " " << hypo.GetCurrSourceRange();

  std::vector<const ChartHypothesis*>::const_iterator iter;
  for (iter = hypo.GetPrevHypos().begin(); iter != hypo.GetPrevHypos().end(); ++iter) {

    const ChartHypothesisMBOT * prevHypo = static_cast<const ChartHypothesisMBOT*> (*iter);
    out << prevHypo->GetId();
  }

  out << " [total=" << hypo.GetTotalScore() << "]";
  out << " " << hypo.GetScoreBreakdown();
  out << std::endl;

  //out << "Processing Phrase : " << hypo.GetProcessingPhrase() << std::endl;

  return out;
}

}
