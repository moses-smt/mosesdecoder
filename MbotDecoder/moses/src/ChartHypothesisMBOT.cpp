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
#include "ChartTranslationOptionMBOT.h"
#include "RuleCubeItemMBOT.h"
#include "FFState.h"

namespace Moses
{

#ifdef USE_HYPO_POOL
ObjectPool<ChartHypothesisMBOT> ChartHypothesisMBOT::s_objectPool("ChartHypothesisMBOT", 300000);
#endif

/** Create a hypothesis from a rule */
ChartHypothesisMBOT::ChartHypothesisMBOT(const ChartTranslationOptionMBOT &mbotTransOpt,
                                 const RuleCubeItemMBOT &mbotItem,
                                 ChartManager &manager)
  :ChartHypothesis(mbotTransOpt, mbotItem, manager)
  ,m_mbotTargetPhrase(const_cast <TargetPhraseMBOT*> (
                                        mbotItem.GetTranslationDimensionMBOT().GetTargetPhraseMBOT()))
  ,m_mbotTransOpt(mbotTransOpt)
  ,m_mbotArcList(NULL)
  ,m_mbotWinningHypo(NULL)
{
  // underlying hypotheses for sub-spans
  const std::vector<HypothesisDimensionMBOT> &childEntries = mbotItem.GetHypothesisDimensionsMBOT();
  m_mbotPrevHypos.reserve(childEntries.size());
  std::vector<HypothesisDimensionMBOT>::const_iterator iter;
  //std::cout << "Iterating over Chart Hypotheses" << std::endl;
  for (iter = childEntries.begin(); iter != childEntries.end(); ++iter)
  {
    m_mbotPrevHypos.push_back(iter->GetHypothesis());
  }
}

ChartHypothesisMBOT::~ChartHypothesisMBOT()
{
  // delete hypotheses that are not in the chart (recombined away)
  if (m_mbotArcList) {
    ChartArcListMBOT::iterator iter;
    for (iter = m_mbotArcList->begin() ; iter != m_mbotArcList->end() ; ++iter) {
      ChartHypothesisMBOT *hypo = *iter;
      DeleteMBOT(hypo);
    }
    m_mbotArcList->clear();

    delete m_mbotArcList;
  }
}

/** Create full output phrase that is contained in the hypothesis (and its children)
 * \param outPhrase full output phrase
 */

//! has to create a possibly discontinuous output phrase
void ChartHypothesisMBOT::CreateOutputPhrase(Phrase &outPhrase, ProcessedNonTerminals & processedNonTerminals) const
{
   processedNonTerminals.AddHypothesis(this);
   processedNonTerminals.AddStatus(this->GetId(),1);

   const ChartHypothesisMBOT * currentHypo = processedNonTerminals.GetHypothesis();
   const std::vector<const AlignmentInfoMBOT*> *alignedTargets = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTAlignments();

        int currentlyProcessed = processedNonTerminals.GetStatus(currentHypo->GetId()) -1;
        CHECK(currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() > currentlyProcessed);
        size_t mbotSize = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size();

        //if several mbot phrases, look at status
         Phrase currentPhrase = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases()[currentlyProcessed];
         int position = 0;
            for (size_t pos = 0; pos < currentPhrase.GetSize(); ++pos) {

                const Word &word = currentPhrase.GetWord(pos);
                if (word.IsNonTerminal()) {
                    const AlignmentInfoMBOT::NonTermIndexMapPointer nonTermIndexMap =
                    alignedTargets->at(currentlyProcessed)->GetNonTermIndexMap();

                    int sizeOfMap = nonTermIndexMap->size();
                size_t nonTermInd = nonTermIndexMap->at(pos);
                processedNonTerminals.IncrementRec();

                const ChartHypothesisMBOT *prevHypo = currentHypo->GetPrevHypoMBOT(nonTermInd);
                prevHypo->CreateOutputPhrase(outPhrase,processedNonTerminals);
                }
                else {
                outPhrase.AddWord(word);
                }
                 if(
                        mbotSize > (processedNonTerminals.GetStatus(currentHypo->GetId()))

                        && (pos == currentPhrase.GetSize() - 1) )
                        {
                            processedNonTerminals.IncrementStatus(currentHypo->GetId());
                            }
        }
        processedNonTerminals.DecrementRec();
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
  // total scores from prev hypos
  std::vector<const ChartHypothesisMBOT*>::iterator iter;
  for (iter = m_mbotPrevHypos.begin(); iter != m_mbotPrevHypos.end(); ++iter) {
    const ChartHypothesisMBOT &prevHypo = **iter;
    const ScoreComponentCollection &scoreBreakdown = prevHypo.GetScoreBreakdown();
    m_scoreBreakdown.PlusEquals(scoreBreakdown);
  }

  // translation models & word penalty
  const ScoreComponentCollection &scoreBreakdown = GetCurrTargetPhraseMBOT().GetScoreBreakdown();
  m_scoreBreakdown.PlusEquals(scoreBreakdown);

  const std::vector<const StatefulFeatureFunction*>& ffs =
    m_manager.GetTranslationSystem()->GetStatefulFeatureFunctions();

  for (unsigned i = 0; i < ffs.size(); ++i) {
		m_ffStates[i] = ffs[i]->EvaluateMBOT(*this,i,&m_scoreBreakdown);
  }

  m_totalScore	= m_scoreBreakdown.GetWeightedScore();
}

void ChartHypothesisMBOT::AddArc(ChartHypothesisMBOT *loserHypo)
{
  if (!m_mbotArcList) {
    if (loserHypo->m_mbotArcList) {
      this->m_mbotArcList = loserHypo->m_mbotArcList;  // take ownership, we'll delete
      loserHypo->m_mbotArcList = 0;                // prevent a double deletion
    } else {
      this->m_mbotArcList = new ChartArcListMBOT();
    }
  } else {
    if (loserHypo->m_mbotArcList) {  // both have an arc list: merge. delete loser
      size_t my_size = m_mbotArcList->size();
      size_t add_size = loserHypo->m_mbotArcList->size();
      this->m_mbotArcList->resize(my_size + add_size, 0);
      std::memcpy(&(*m_mbotArcList)[0] + my_size, &(*loserHypo->m_mbotArcList)[0], add_size * sizeof(ChartHypothesisMBOT *));
      delete loserHypo->m_mbotArcList;
      loserHypo->m_mbotArcList = 0;
    } else { // loserHypo doesn't have any arcs
      // DO NOTHING
    }
  }
  m_mbotArcList->push_back(loserHypo);
}

struct CompareChartChartHypothesisMBOTTotalScore {
  bool operator()(const ChartHypothesis* hypo1, const ChartHypothesis* hypo2) const {
	if(hypo1 != NULL && hypo2 != NULL)
    {
		return hypo1->GetTotalScore() > hypo2->GetTotalScore();
    }
  }
};

void ChartHypothesisMBOT::CleanupArcList()
{
  // point this hypo's main hypo to itself
  m_mbotWinningHypo = this;

  if (!m_mbotArcList)
  {
        return;
  }

  /* keep only number of arcs we need to create all n-best paths.
   * However, may not be enough if only unique candidates are needed,
   * so we'll keep all of arc list if nedd distinct n-best list
   */
  const StaticData &staticData = StaticData::Instance();
  size_t nBestSize = staticData.GetNBestSize();
  bool distinctNBest = staticData.GetDistinctNBest() || staticData.UseMBR() || staticData.GetOutputSearchGraph();

  //Fabienne Braune : Pruning disabled for MBOT hypotheses
  /*	if (!distinctNBest && m_mbotArcList->size() > nBestSize) {
    	nth_element(m_mbotArcList->begin()
                , m_mbotArcList->begin() + nBestSize - 1
                , m_mbotArcList->end()
                , CompareChartChartHypothesisMBOTTotalScore());

    // delete bad ones
    ChartArcListMBOT::iterator iter;
    for (iter = m_mbotArcList->begin() + nBestSize ; iter != m_mbotArcList->end() ; ++iter) {
      ChartHypothesisMBOT *arc = *iter;
      ChartHypothesisMBOT::DeleteMBOT(arc);
    }
    m_mbotArcList->erase(m_mbotArcList->begin() + nBestSize
                     , m_mbotArcList->end());
  }*/

  // set all arc's main hypo variable to this hypo
  ChartArcListMBOT::iterator iter = m_mbotArcList->begin();
  for (; iter != m_mbotArcList->end() ; ++iter) {
    //std::cout << "Trying to get arc" << std::endl;
    ChartHypothesisMBOT *arc = *iter;
    arc->SetWinningHypo(this);
  }
}

void ChartHypothesisMBOT::SetWinningHypo(const ChartHypothesisMBOT *hypo)
{
  m_mbotWinningHypo = hypo;
}

TO_STRING_BODY(ChartHypothesisMBOT)

// friend
std::ostream& operator<<(std::ostream& out, const ChartHypothesisMBOT& hypo)
{

  out << hypo.GetId();

	// recombination
	if (hypo.GetWinningHypothesisMBOT() != NULL &&
			hypo.GetWinningHypothesisMBOT() != &hypo)
	{
		out << "->" << hypo.GetWinningHypothesisMBOT()->GetId();
	}
    out  << " " << hypo.GetCurrSourceRange();

  std::vector<const ChartHypothesisMBOT*>::const_iterator iter;
  for (iter = hypo.GetPrevHyposMBOT().begin(); iter != hypo.GetPrevHyposMBOT().end(); ++iter) {

    const ChartHypothesisMBOT * prevHypo = *(iter);
    out << prevHypo->GetId();
  }

  out << " [total=" << hypo.GetTotalScore() << "]";
  out << " " << hypo.GetScoreBreakdown();
  StaticData::Instance().GetScoreIndexManager().PrintLabeledScores(out,hypo.GetScoreBreakdown());
  out << std::endl;

  return out;
}

}
