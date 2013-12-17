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
#include "ChartHypothesis.h"
#include "RuleCubeItem.h"
#include "ChartCell.h"
#include "ChartManager.h"
#include "TargetPhrase.h"
#include "Phrase.h"
#include "StaticData.h"
#include "ChartTranslationOptions.h"
#include "moses/FF/FFState.h"

using namespace std;

namespace Moses
{

#ifdef USE_HYPO_POOL
ObjectPool<ChartHypothesis> ChartHypothesis::s_objectPool("ChartHypothesis", 300000);
#endif

/** Create a hypothesis from a rule
 * \param transOpt wrapper around the rule
 * \param item @todo dunno
 * \param manager reference back to manager
 */
ChartHypothesis::ChartHypothesis(const ChartTranslationOptions &transOpt,
                                 const RuleCubeItem &item,
                                 ChartManager &manager)
  :m_transOpt(item.GetTranslationDimension().GetTranslationOption())
  ,m_currSourceWordsRange(transOpt.GetSourceWordsRange())
  ,m_ffStates(StatefulFeatureFunction::GetStatefulFeatureFunctions().size())
  ,m_arcList(NULL)
  ,m_winningHypo(NULL)
  ,m_manager(manager)
  ,m_id(manager.GetNextHypoId())
{
  // underlying hypotheses for sub-spans
  const std::vector<HypothesisDimension> &childEntries = item.GetHypothesisDimensions();
  m_prevHypos.reserve(childEntries.size());
  std::vector<HypothesisDimension>::const_iterator iter;
  for (iter = childEntries.begin(); iter != childEntries.end(); ++iter) {
    m_prevHypos.push_back(iter->GetHypothesis());
  }
}

ChartHypothesis::~ChartHypothesis()
{
  // delete feature function states
  for (unsigned i = 0; i < m_ffStates.size(); ++i) {
    delete m_ffStates[i];
  }

  // delete hypotheses that are not in the chart (recombined away)
  if (m_arcList) {
    ChartArcList::iterator iter;
    for (iter = m_arcList->begin() ; iter != m_arcList->end() ; ++iter) {
      ChartHypothesis *hypo = *iter;
      Delete(hypo);
    }
    m_arcList->clear();

    delete m_arcList;
  }
}

/** Create full output phrase that is contained in the hypothesis (and its children)
 * \param outPhrase full output phrase as return argument
 */
void ChartHypothesis::GetOutputPhrase(Phrase &outPhrase) const
{
  FactorType placeholderFactor = StaticData::Instance().GetPlaceholderFactor();

  for (size_t pos = 0; pos < GetCurrTargetPhrase().GetSize(); ++pos) {
    const Word &word = GetCurrTargetPhrase().GetWord(pos);
    if (word.IsNonTerminal()) {
      // non-term. fill out with prev hypo
      size_t nonTermInd = GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
      const ChartHypothesis *prevHypo = m_prevHypos[nonTermInd];
      prevHypo->GetOutputPhrase(outPhrase);
    } else {
      outPhrase.AddWord(word);

      if (placeholderFactor != NOT_FOUND) {
    	  std::set<size_t> sourcePosSet = GetCurrTargetPhrase().GetAlignTerm().GetAlignmentsForTarget(pos);
    	  if (sourcePosSet.size() == 1) {
    		  const std::vector<const Word*> *ruleSourceFromInputPath = GetTranslationOption().GetSourceRuleFromInputPath();
    		  CHECK(ruleSourceFromInputPath);

    	      size_t sourcePos = *sourcePosSet.begin();
    		  const Word *sourceWord = ruleSourceFromInputPath->at(sourcePos);
    		  CHECK(sourceWord);
    		  const Factor *factor = sourceWord->GetFactor(placeholderFactor);
    		  if (factor) {
    			  outPhrase.Back()[0] = factor;
    		  }
     	  }
      }

    }
  }
}

/** Return full output phrase */
Phrase ChartHypothesis::GetOutputPhrase() const
{
  Phrase outPhrase(ARRAY_SIZE_INCR);
  GetOutputPhrase(outPhrase);
  return outPhrase;
}

/** check, if two hypothesis can be recombined.
    this is actually a sorting function that allows us to
    keep an ordered list of hypotheses. This makes recombination
    much quicker. Returns one of 3 possible values:
      -1 = this < compare
      +1 = this > compare
      0	= this ==compare
 \param compare the other hypo to compare to
*/
int ChartHypothesis::RecombineCompare(const ChartHypothesis &compare) const
{
  int comp = 0;

  for (unsigned i = 0; i < m_ffStates.size(); ++i) {
    if (m_ffStates[i] == NULL || compare.m_ffStates[i] == NULL)
      comp = m_ffStates[i] - compare.m_ffStates[i];
    else
      comp = m_ffStates[i]->Compare(*compare.m_ffStates[i]);

    if (comp != 0)
      return comp;
  }

  return 0;
}

/** calculate total score
  * @todo this should be in ScoreBreakdown
 */
void ChartHypothesis::Evaluate()
{

  //std::cerr << "SCORE COMPONENT BEFORE EVALUATE : " << m_scoreBreakdown << std::endl;

  const StaticData &staticData = StaticData::Instance();
  // total scores from prev hypos
  std::vector<const ChartHypothesis*>::iterator iter;
  for (iter = m_prevHypos.begin(); iter != m_prevHypos.end(); ++iter) {
    const ChartHypothesis &prevHypo = **iter;
    const ScoreComponentCollection &scoreBreakdown = prevHypo.GetScoreBreakdown();

    m_scoreBreakdown.PlusEquals(scoreBreakdown);
  }

  // scores from current translation rule. eg. translation models & word penalty
  //Fabienne Braune : Outcommented in this branch because I need the scores of the whole translation option, not just the target phrase
  //const ScoreComponentCollection &scoreBreakdown = GetCurrTargetPhrase().GetScoreBreakdown();
  //  m_scoreBreakdown.PlusEquals(scoreBreakdown);

  // scores from current translation rule. eg. translation models & word penalty and others from translation options
   const ScoreComponentCollection &scoreBreakdown = GetTranslationOption().GetScores();
   m_scoreBreakdown.PlusEquals(scoreBreakdown);

  // compute values of stateless feature functions that were not
  // cached in the translation option-- there is no principled distinction
  const std::vector<const StatelessFeatureFunction*>& sfs =
    StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for (unsigned i = 0; i < sfs.size(); ++i) {
    if (! staticData.IsFeatureFunctionIgnored( *sfs[i] )) {
      sfs[i]->EvaluateChart(*this,&m_scoreBreakdown);
    }
  }

  const std::vector<const StatefulFeatureFunction*>& ffs =
    StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i) {
    if (! staticData.IsFeatureFunctionIgnored( *ffs[i] )) {
      m_ffStates[i] = ffs[i]->EvaluateChart(*this,i,&m_scoreBreakdown);
    }
  }

  m_totalScore	= m_scoreBreakdown.GetWeightedScore();
}

void ChartHypothesis::AddArc(ChartHypothesis *loserHypo)
{
  if (!m_arcList) {
    if (loserHypo->m_arcList) { // we don't have an arcList, but loser does
      this->m_arcList = loserHypo->m_arcList;  // take ownership, we'll delete
      loserHypo->m_arcList = 0;                // prevent a double deletion
    } else {
      this->m_arcList = new ChartArcList();
    }
  } else {
    if (loserHypo->m_arcList) {  // both have an arc list: merge. delete loser
      size_t my_size = m_arcList->size();
      size_t add_size = loserHypo->m_arcList->size();
      this->m_arcList->resize(my_size + add_size, 0);
      std::memcpy(&(*m_arcList)[0] + my_size, &(*loserHypo->m_arcList)[0], add_size * sizeof(ChartHypothesis *));
      delete loserHypo->m_arcList;
      loserHypo->m_arcList = 0;
    } else { // loserHypo doesn't have any arcs
      // DO NOTHING
    }
  }
  m_arcList->push_back(loserHypo);
}

// sorting helper
struct CompareChartChartHypothesisTotalScore {
  bool operator()(const ChartHypothesis* hypo1, const ChartHypothesis* hypo2) const
  {
    return hypo1->GetTotalScore() > hypo2->GetTotalScore();
  }
};

void ChartHypothesis::CleanupArcList()
{
  // point this hypo's main hypo to itself
  m_winningHypo = this;

  if (!m_arcList) return;

  /* keep only number of arcs we need to create all n-best paths.
   * However, may not be enough if only unique candidates are needed,
   * so we'll keep all of arc list if nedd distinct n-best list
   */
  const StaticData &staticData = StaticData::Instance();
  size_t nBestSize = staticData.GetNBestSize();
  bool distinctNBest = staticData.GetDistinctNBest() || staticData.UseMBR() || staticData.GetOutputSearchGraph();

  //Fabienne Braune : this is broken but no idea why...
  /*if (!distinctNBest && m_arcList->size() > nBestSize) {

	std::cerr << "Size of arc list : " << m_arcList->size() << std::endl;

	//For debugging
	/*int counter = 0;
	vector<ChartHypothesis*> :: iterator itr_hypos;
	for(itr_hypos = m_arcList->begin();itr_hypos != m_arcList->begin() + nBestSize - 1; itr_hypos++)
	{
		std::cerr << "EXISTING HYPO : " << (*itr_hypos)->GetCurrTargetPhrase() << std::endl;
	}*/

    // prune arc list only if there too many arcs
    /*nth_element(m_arcList->begin()
                , m_arcList->begin() + nBestSize - 1
                , m_arcList->end()
                , CompareChartChartHypothesisTotalScore());

    // delete bad ones
    ChartArcList::iterator iter;
    for (iter = m_arcList->begin() + nBestSize ; iter != m_arcList->end() ; ++iter) {
      ChartHypothesis *arc = *iter;
      //Check if hypo is NULL
      if(arc != NULL)
      {ChartHypothesis::Delete(arc);}
    }
    m_arcList->erase(m_arcList->begin() + nBestSize
                     , m_arcList->end());
  }*/

  // set all arc's main hypo variable to this hypo
  ChartArcList::iterator iter = m_arcList->begin();
  for (; iter != m_arcList->end() ; ++iter) {
    ChartHypothesis *arc = *iter;
    arc->SetWinningHypo(this);
  }

  //cerr << m_arcList->size() << " ";
}

void ChartHypothesis::SetWinningHypo(const ChartHypothesis *hypo)
{
  m_winningHypo = hypo;
}

TO_STRING_BODY(ChartHypothesis)

// friend
std::ostream& operator<<(std::ostream& out, const ChartHypothesis& hypo)
{

  out << hypo.GetId();

  // recombination
  if (hypo.GetWinningHypothesis() != NULL &&
      hypo.GetWinningHypothesis() != &hypo) {
    out << "->" << hypo.GetWinningHypothesis()->GetId();
  }

  if (StaticData::Instance().GetIncludeLHSInSearchGraph()) {
    out << " " << hypo.GetTargetLHS() << "=>";
  }
  out << " " << hypo.GetCurrTargetPhrase()
      //<< " " << outPhrase
      << " " << hypo.GetCurrSourceRange();

  HypoList::const_iterator iter;
  for (iter = hypo.GetPrevHypos().begin(); iter != hypo.GetPrevHypos().end(); ++iter) {
    const ChartHypothesis &prevHypo = **iter;
    out << " " << prevHypo.GetId();
  }

  out << " [total=" << hypo.GetTotalScore() << "]";
  out << " " << hypo.GetScoreBreakdown();

  //out << endl;

  return out;
}

}
