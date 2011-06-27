// $Id$
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
#include "RuleCube.h"
#include "ChartCell.h"
#include "ChartManager.h"
#include "TargetPhrase.h"
#include "Phrase.h"
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "LMList.h"
#include "ChartTranslationOption.h"
#include "FFState.h"

using namespace std;
using namespace Moses;

namespace Moses
{
unsigned int ChartHypothesis::s_HypothesesCreated = 0;

#ifdef USE_HYPO_POOL
ObjectPool<ChartHypothesis> ChartHypothesis::s_objectPool("ChartHypothesis", 300000);
#endif

/** Create a hypothesis from a rule */
ChartHypothesis::ChartHypothesis(const RuleCube &ruleCube, ChartManager &manager)
  :m_transOpt(ruleCube.GetTranslationOption())
  ,m_id(++s_HypothesesCreated)
  ,m_currSourceWordsRange(ruleCube.GetTranslationOption().GetSourceWordsRange())
	,m_ffStates(manager.GetTranslationSystem()->GetStatefulFeatureFunctions().size())
  ,m_contextPrefix(Output, manager.GetTranslationSystem()->GetLanguageModels().GetMaxNGramOrder())
  ,m_contextSuffix(Output, manager.GetTranslationSystem()->GetLanguageModels().GetMaxNGramOrder())
  ,m_arcList(NULL)
	,m_winningHypo(NULL)
  ,m_manager(manager)
{
  //TRACE_ERR(m_targetPhrase << endl);

  // underlying hypotheses for sub-spans
  m_numTargetTerminals = GetCurrTargetPhrase().GetNumTerminals();
  const std::vector<RuleCubeDimension> &childEntries = ruleCube.GetCube();

  // ... are stored
  assert(m_prevHypos.empty());
  m_prevHypos.reserve(childEntries.size());

  vector<RuleCubeDimension>::const_iterator iter;
  for (iter = childEntries.begin(); iter != childEntries.end(); ++iter) 
  {
    const RuleCubeDimension &ruleCubeDimension = *iter;
    const ChartHypothesis *prevHypo = ruleCubeDimension.GetHypothesis();

    // keep count of words (= length of generated string)
    m_numTargetTerminals += prevHypo->GetNumTargetTerminals();

    m_prevHypos.push_back(prevHypo);
  }

  // compute the relevant context for language model scoring (prefix and suffix strings)
  size_t maxNGram = manager.GetTranslationSystem()->GetLanguageModels().GetMaxNGramOrder();
  CalcPrefix(m_contextPrefix, maxNGram - 1);
  CalcSuffix(m_contextSuffix, maxNGram - 1);
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
 * \param outPhrase full output phrase
 */
void ChartHypothesis::CreateOutputPhrase(Phrase &outPhrase) const
{
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    GetCurrTargetPhrase().GetAlignmentInfo().GetNonTermIndexMap();

  for (size_t pos = 0; pos < GetCurrTargetPhrase().GetSize(); ++pos) {
    const Word &word = GetCurrTargetPhrase().GetWord(pos);
    if (word.IsNonTerminal()) {
      // non-term. fill out with prev hypo
      size_t nonTermInd = nonTermIndexMap[pos];
      const ChartHypothesis *prevHypo = m_prevHypos[nonTermInd];
      prevHypo->CreateOutputPhrase(outPhrase);
    } 
    else {
      outPhrase.AddWord(word);
    }
  }
}

/** Return full output phrase */
Phrase ChartHypothesis::GetOutputPhrase() const
{
  Phrase outPhrase(Output, ARRAY_SIZE_INCR);
  CreateOutputPhrase(outPhrase);
  return outPhrase;
}

/** Construct the prefix string of up to specified size 
 * \param ret prefix string
 * \param size maximum size (typically max lm context window)
 */
size_t ChartHypothesis::CalcPrefix(Phrase &ret, size_t size) const
{
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    GetCurrTargetPhrase().GetAlignmentInfo().GetNonTermIndexMap();

  // loop over the rule that is being applied
  for (size_t pos = 0; pos < GetCurrTargetPhrase().GetSize(); ++pos) {
    const Word &word = GetCurrTargetPhrase().GetWord(pos);

    // for non-terminals, retrieve it from underlying hypothesis
    if (word.IsNonTerminal()) {
      size_t nonTermInd = nonTermIndexMap[pos];
      const ChartHypothesis *prevHypo = m_prevHypos[nonTermInd];
      size = prevHypo->CalcPrefix(ret, size);
    } 
    // for words, add word
    else {
      ret.AddWord(GetCurrTargetPhrase().GetWord(pos));
      size--;
    }

    // finish when maximum length reached
    if (size==0)
      break;
  }

  return size;
}

/** Construct the suffix phrase of up to specified size 
 * will always be called after the construction of prefix phrase
 * \param ret suffix phrase
 * \param size maximum size of suffix
 */
size_t ChartHypothesis::CalcSuffix(Phrase &ret, size_t size) const
{
  assert(m_contextPrefix.GetSize() <= m_numTargetTerminals);

  // special handling for small hypotheses
  // does the prefix match the entire hypothesis string? -> just copy prefix
  if (m_contextPrefix.GetSize() == m_numTargetTerminals) {
    size_t maxCount = min(m_contextPrefix.GetSize(), size)
                      , pos			= m_contextPrefix.GetSize() - 1;

    for (size_t ind = 0; ind < maxCount; ++ind) {
      const Word &word = m_contextPrefix.GetWord(pos);
      ret.PrependWord(word);
      --pos;
    }

    size -= maxCount;
    return size;
  } 
  // construct suffix analogous to prefix
  else {
    const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      GetCurrTargetPhrase().GetAlignmentInfo().GetNonTermIndexMap();
    for (int pos = (int) GetCurrTargetPhrase().GetSize() - 1; pos >= 0 ; --pos) {
      const Word &word = GetCurrTargetPhrase().GetWord(pos);

      if (word.IsNonTerminal()) {
        size_t nonTermInd = nonTermIndexMap[pos];
        const ChartHypothesis *prevHypo = m_prevHypos[nonTermInd];
        size = prevHypo->CalcSuffix(ret, size);
      } 
      else {
        ret.PrependWord(GetCurrTargetPhrase().GetWord(pos));
        size--;
      }

      if (size==0)
        break;
    }

    return size;
  }
}

/** check, if two hypothesis can be recombined.
    this is actually a sorting function that allows us to
    keep an ordered list of hypotheses. This makes recombination
    much quicker.
*/
int ChartHypothesis::RecombineCompare(const ChartHypothesis &compare) const
{
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

  return 0;
}

void ChartHypothesis::CalcScore()
{
  // total scores from prev hypos
  std::vector<const ChartHypothesis*>::iterator iter;
  for (iter = m_prevHypos.begin(); iter != m_prevHypos.end(); ++iter) {
    const ChartHypothesis &prevHypo = **iter;
    const ScoreComponentCollection &scoreBreakdown = prevHypo.GetScoreBreakdown();

    m_scoreBreakdown.PlusEquals(scoreBreakdown);
  }

  // translation models & word penalty
  const ScoreComponentCollection &scoreBreakdown = GetCurrTargetPhrase().GetScoreBreakdown();
  m_scoreBreakdown.PlusEquals(scoreBreakdown);

	// compute values of stateless feature functions that were not
  // cached in the translation option-- there is no principled distinction

  //const vector<const StatelessFeatureFunction*>& sfs =
  //  m_manager.GetTranslationSystem()->GetStatelessFeatureFunctions();
	// TODO!
  //for (unsigned i = 0; i < sfs.size(); ++i) {
  //  sfs[i]->ChartEvaluate(m_targetPhrase, &m_scoreBreakdown);
  //}

  const vector<const StatefulFeatureFunction*>& ffs =
    m_manager.GetTranslationSystem()->GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i) {
		m_ffStates[i] = ffs[i]->EvaluateChart(*this,i,&m_scoreBreakdown);
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
  bool operator()(const ChartHypothesis* hypo1, const ChartHypothesis* hypo2) const {
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

  if (!distinctNBest && m_arcList->size() > nBestSize) {
    // prune arc list only if there too many arcs
    nth_element(m_arcList->begin()
                , m_arcList->begin() + nBestSize - 1
                , m_arcList->end()
                , CompareChartChartHypothesisTotalScore());

    // delete bad ones
    ChartArcList::iterator iter;
    for (iter = m_arcList->begin() + nBestSize ; iter != m_arcList->end() ; ++iter) {
      ChartHypothesis *arc = *iter;
      ChartHypothesis::Delete(arc);
    }
    m_arcList->erase(m_arcList->begin() + nBestSize
                     , m_arcList->end());
  }

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

  // never gonna use to recombine. clear prefix & suffix phrases to save mem
  m_contextPrefix.Clear();
  m_contextSuffix.Clear();
}

TO_STRING_BODY(ChartHypothesis)

// friend
ostream& operator<<(ostream& out, const ChartHypothesis& hypo)
{

  out << hypo.GetId();
	
	// recombination
	if (hypo.GetWinningHypothesis() != NULL &&
			hypo.GetWinningHypothesis()->GetId() != hypo.GetId())
	{
		out << "->" << hypo.GetWinningHypothesis()->GetId();
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

