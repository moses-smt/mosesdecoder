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
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"

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
  for (iter = childEntries.begin(); iter != childEntries.end(); ++iter) 
  {
    m_prevHypos.push_back(iter->GetHypothesis());
  }
}

// Intended to be used by ChartKBestExtractor only.  This creates a mock
// ChartHypothesis for use by the extractor's top-level target vertex.
ChartHypothesis::ChartHypothesis(const ChartHypothesis &pred,
                                 const ChartKBestExtractor & /*unused*/)
  :m_currSourceWordsRange(pred.m_currSourceWordsRange)
  ,m_totalScore(pred.m_totalScore)
  ,m_arcList(NULL)
  ,m_winningHypo(NULL)
  ,m_manager(pred.m_manager)
  ,m_id(pred.m_manager.GetNextHypoId())
{
  // One predecessor, which is an existing top-level ChartHypothesis.
  m_prevHypos.push_back(&pred);
}

ChartHypothesis::~ChartHypothesis()
{
  // delete feature function states
  for (unsigned i = 0; i < m_ffStates.size(); ++i) 
  {
    delete m_ffStates[i];
  }

  // delete hypotheses that are not in the chart (recombined away)
  if (m_arcList) 
  {
    ChartArcList::iterator iter;
    for (iter = m_arcList->begin() ; iter != m_arcList->end() ; ++iter) 
    {
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

  for (size_t pos = 0; pos < GetCurrTargetPhrase().GetSize(); ++pos) 
  {
    const Word &word = GetCurrTargetPhrase().GetWord(pos);
    if (word.IsNonTerminal()) 
    {
      // non-term. fill out with prev hypo
      size_t nonTermInd = GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
      const ChartHypothesis *prevHypo = m_prevHypos[nonTermInd];
      prevHypo->GetOutputPhrase(outPhrase);
    } 
    else 
    {
      outPhrase.AddWord(word);

      if (placeholderFactor != NOT_FOUND) 
      {
        std::set<size_t> sourcePosSet = GetCurrTargetPhrase().GetAlignTerm().GetAlignmentsForTarget(pos);
        if (sourcePosSet.size() == 1) 
        {
          const std::vector<const Word*> *ruleSourceFromInputPath = GetTranslationOption().GetSourceRuleFromInputPath();
          UTIL_THROW_IF2(ruleSourceFromInputPath == NULL,
                         "No source rule");

          size_t sourcePos = *sourcePosSet.begin();
          const Word *sourceWord = ruleSourceFromInputPath->at(sourcePos);
          UTIL_THROW_IF2(sourceWord == NULL,
                         "No source word");
          const Factor *factor = sourceWord->GetFactor(placeholderFactor);
          if (factor) 
          {
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

/** TODO: this method isn't used anywhere. Remove? */
void ChartHypothesis::GetOutputPhrase(size_t leftRightMost, size_t numWords, Phrase &outPhrase) const
{
  const TargetPhrase &tp = GetCurrTargetPhrase();

  size_t targetSize = tp.GetSize();
  for (size_t i = 0; i < targetSize; ++i) 
  {
    size_t pos;
    if (leftRightMost == 1) 
    {
      pos = i;
    }
    else if (leftRightMost == 2) 
    {
      pos = targetSize - i - 1;
    }
    else 
    {
      abort();
    }

    const Word &word = tp.GetWord(pos);

    if (word.IsNonTerminal()) 
    {
      // non-term. fill out with prev hypo
      size_t nonTermInd = tp.GetAlignNonTerm().GetNonTermIndexMap()[pos];
      const ChartHypothesis *prevHypo = m_prevHypos[nonTermInd];
      prevHypo->GetOutputPhrase(outPhrase);
    } 
    else 
    {
      outPhrase.AddWord(word);
    }

    if (outPhrase.GetSize() >= numWords) {
      return;
    }
  }
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

/** calculate total score */
void ChartHypothesis::EvaluateWhenApplied()
{
  const StaticData &staticData = StaticData::Instance();

  // compute values of stateless feature functions that were not
  // cached in the translation option-- there is no principled distinction
  const std::vector<const StatelessFeatureFunction*>& sfs =
    StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for (unsigned i = 0; i < sfs.size(); ++i) 
  {
    if (! staticData.IsFeatureFunctionIgnored( *sfs[i] )) 
    {
      sfs[i]->EvaluateWhenApplied(*this,&m_currScoreBreakdown);
    }
  }

  const std::vector<const StatefulFeatureFunction*>& ffs =
    StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i) 
  {
    if (! staticData.IsFeatureFunctionIgnored( *ffs[i] )) 
    {
      m_ffStates[i] = ffs[i]->EvaluateWhenApplied(*this,i,&m_currScoreBreakdown);
    }
  }

  // total score from current translation rule
  m_totalScore = GetTranslationOption().GetScores().GetWeightedScore();
  m_totalScore += m_currScoreBreakdown.GetWeightedScore();
 
  // total scores from prev hypos
  for (std::vector<const ChartHypothesis*>::const_iterator iter = m_prevHypos.begin(); iter != m_prevHypos.end(); ++iter) {
    const ChartHypothesis &prevHypo = **iter;
    m_totalScore += prevHypo.GetTotalScore();
  }
}

void ChartHypothesis::AddArc(ChartHypothesis *loserHypo)
{
  if (!m_arcList) 
  {
    if (loserHypo->m_arcList) 
    { // we don't have an arcList, but loser does
      this->m_arcList = loserHypo->m_arcList;  // take ownership, we'll delete
      loserHypo->m_arcList = 0;                // prevent a double deletion
    } 
    else 
    {
      this->m_arcList = new ChartArcList();
    }
  } 
  else 
  {
    if (loserHypo->m_arcList) 
    {  // both have an arc list: merge. delete loser
      size_t my_size = m_arcList->size();
      size_t add_size = loserHypo->m_arcList->size();
      this->m_arcList->resize(my_size + add_size, 0);
      std::memcpy(&(*m_arcList)[0] + my_size, &(*loserHypo->m_arcList)[0], add_size * sizeof(ChartHypothesis *));
      delete loserHypo->m_arcList;
      loserHypo->m_arcList = 0;
    } 
    else 
    { // loserHypo doesn't have any arcs
      // DO NOTHING
    }
  }
  m_arcList->push_back(loserHypo);
}

// sorting helper
struct CompareChartHypothesisTotalScore 
{
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
  bool distinctNBest = staticData.GetDistinctNBest() || staticData.UseMBR() || staticData.GetOutputSearchGraph() || staticData.GetOutputSearchGraphHypergraph();

  if (!distinctNBest && m_arcList->size() > nBestSize) 
  {
    // prune arc list only if there too many arcs
    NTH_ELEMENT4(m_arcList->begin()
                 , m_arcList->begin() + nBestSize - 1
                 , m_arcList->end()
                 , CompareChartHypothesisTotalScore());

    // delete bad ones
    ChartArcList::iterator iter;
    for (iter = m_arcList->begin() + nBestSize ; iter != m_arcList->end() ; ++iter) 
    {
      ChartHypothesis *arc = *iter;
      ChartHypothesis::Delete(arc);
    }
    m_arcList->erase(m_arcList->begin() + nBestSize
                     , m_arcList->end());
  }

  // set all arc's main hypo variable to this hypo
  ChartArcList::iterator iter = m_arcList->begin();
  for (; iter != m_arcList->end() ; ++iter) 
  {
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
      hypo.GetWinningHypothesis() != &hypo) 
  {
    out << "->" << hypo.GetWinningHypothesis()->GetId();
  }

  if (StaticData::Instance().GetIncludeLHSInSearchGraph()) 
  {
    out << " " << hypo.GetTargetLHS() << "=>";
  }
  out << " " << hypo.GetCurrTargetPhrase()
      //<< " " << outPhrase
      << " " << hypo.GetCurrSourceRange();

  HypoList::const_iterator iter;
  for (iter = hypo.GetPrevHypos().begin(); iter != hypo.GetPrevHypos().end(); ++iter) 
  {
    const ChartHypothesis &prevHypo = **iter;
    out << " " << prevHypo.GetId();
  }

  out << " [total=" << hypo.GetTotalScore() << "]";
  out << " " << hypo.GetScoreBreakdown();

  //out << endl;

  return out;
}

}
