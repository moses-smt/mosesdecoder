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
  ,m_coveredChartSpanListTargetOrder(ruleCube.GetTranslationOption().GetCoveredChartSpanTargetOrder())
  ,m_id(++s_HypothesesCreated)
  ,m_currSourceWordsRange(ruleCube.GetTranslationOption().GetSourceWordsRange())
  ,m_contextPrefix(Output, manager.GetTranslationSystem()->GetLanguageModels().GetMaxNGramOrder())
  ,m_contextSuffix(Output, manager.GetTranslationSystem()->GetLanguageModels().GetMaxNGramOrder())
  ,m_arcList(NULL)
  ,m_manager(manager)
{
  assert(GetCurrTargetPhrase().GetSize() == m_coveredChartSpanListTargetOrder.size());
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
  for (size_t pos = 0; pos < GetCurrTargetPhrase().GetSize(); ++pos) {
    const Word &word = GetCurrTargetPhrase().GetWord(pos);
    if (word.IsNonTerminal()) {
      // non-term. fill out with prev hypo
      size_t nonTermInd = m_coveredChartSpanListTargetOrder[pos];
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
  // loop over the rule that is being applied
  for (size_t pos = 0; pos < GetCurrTargetPhrase().GetSize(); ++pos) {
    const Word &word = GetCurrTargetPhrase().GetWord(pos);

    // for non-terminals, retrieve it from underlying hypothesis
    if (word.IsNonTerminal()) {
      size_t nonTermInd = m_coveredChartSpanListTargetOrder[pos];
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
    for (int pos = (int) GetCurrTargetPhrase().GetSize() - 1; pos >= 0 ; --pos) {
      const Word &word = GetCurrTargetPhrase().GetWord(pos);

      if (word.IsNonTerminal()) {
        size_t nonTermInd = m_coveredChartSpanListTargetOrder[pos];
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

/** Check recombinability due to language model context
 * if two hypotheses match in their edge phrases, they are indistinguishable
 * in terms of future search, so the weaker one can be dropped.
 * \param other other hypothesis for comparison
 */
int ChartHypothesis::LMContextCompare(const ChartHypothesis &other) const
{
  // prefix
  if (m_currSourceWordsRange.GetStartPos() > 0) {
    int ret = GetPrefix().Compare(other.GetPrefix());
    if (ret != 0)
      return ret;
  }

  // suffix
  size_t inputSize = m_manager.GetSource().GetSize();
  if (m_currSourceWordsRange.GetEndPos() < inputSize - 1) {
    int ret = GetSuffix().Compare(other.GetSuffix());
    if (ret != 0)
      return ret;
  }

  // they're the same
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

  CalcLMScore();

  m_totalScore	= m_scoreBreakdown.GetWeightedScore();
}

// compute languane model score for the hypothesis
void ChartHypothesis::CalcLMScore()
{
  // get the language models involved
  const LMList& lmList = m_manager.GetTranslationSystem()->GetLanguageModels();
  assert(m_lmNGram.GetWeightedScore() == 0);

  // start scores with 0
  m_scoreBreakdown.ZeroAllLM(lmList);

  // initialize output string (empty)
  Phrase outPhrase(Output, ARRAY_SIZE_INCR);
  bool firstPhrase = true; // flag for first word

  // loop over rule
  for (size_t targetPhrasePos = 0; targetPhrasePos < GetCurrTargetPhrase().GetSize(); ++targetPhrasePos) 
  {
    // consult rule for either word or non-terminal
    const Word &targetWord = GetCurrTargetPhrase().GetWord(targetPhrasePos);

    // just a word, add it to phrase for lm scoring
    if (!targetWord.IsNonTerminal()) 
    {
      outPhrase.AddWord(targetWord);
    } 
    // non-terminal, add phrase from underlying hypothesis
    else 
    {
      // look up underlying hypothesis
      size_t nonTermInd = m_coveredChartSpanListTargetOrder[targetPhrasePos];
      const ChartHypothesis *prevHypo = m_prevHypos[nonTermInd];
      size_t numTargetTerminals = prevHypo->GetNumTargetTerminals();

      // check if we are dealing with a large sub-phrase
      // (large sub-phrases have internal language model scores)
      if (numTargetTerminals < lmList.GetMaxNGramOrder() - 1) 
      {
        // small sub-phrase (for trigram lm, 1 or 2-word hypos).
        // add target phrase to temp phrase and continue, but don't score yet
        outPhrase.Append(prevHypo->GetPrefix());
      }
      else {
        // large sub-phrase
        // we add the internal scores
        m_lmNGram.PlusEqualsAllLM(lmList, prevHypo->m_lmNGram);

        // we will still have to score the prefix
        outPhrase.Append(prevHypo->GetPrefix());

        // preliminary language model scoring

        // special case: rule starts with non-terminal
        if (targetPhrasePos == 0 && numTargetTerminals >= lmList.GetMaxNGramOrder() - 1) {
          // get from other prev hypo. faster
          m_lmPrefix.Assign(prevHypo->m_lmPrefix);
          m_lmNGram.Assign(prevHypo->m_lmNGram);
        } 
        // general case
        else {
          // score everything we got so far
          lmList.CalcAllLMScores(outPhrase
                                 , m_lmNGram
                                 , (firstPhrase) ? &m_lmPrefix : NULL);
        }

        // create new phrase from suffix. score later when appended with next words
        outPhrase.Clear();
        outPhrase.Append(prevHypo->GetSuffix());

        firstPhrase = false;
      } // if long sub-phrase
    } // if (!targetWord.IsNonTerminal())
  } // for (size_t targetPhrasePos

  lmList.CalcAllLMScores(outPhrase
                         , m_lmNGram
                         , (firstPhrase) ? &m_lmPrefix : NULL);

  // add score estimate for prefix
  m_scoreBreakdown.PlusEqualsAllLM(lmList, m_lmPrefix);
  // add real score for words with full history
  m_scoreBreakdown.PlusEqualsAllLM(lmList, m_lmNGram);

  /*
  	// lazy way. keep for comparison
  	Phrase outPhrase = GetOutputPhrase();
  //	cerr << outPhrase << " ";

  	float retFullScore, retNGramScore;
  	StaticData::Instance().GetAllLM().CalcScore(outPhrase
  																						, retFullScore
  																						, retNGramScore
  																						, m_scoreBreakdown
  																						, &m_lmNGram
  																						, false);
  */
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
  //Phrase outPhrase(Output);
  //hypo.CreateOutputPhrase(outPhrase);

  // words bitmap
  out << " " << hypo.GetId()
      << " " << hypo.GetCurrTargetPhrase()
      //<< " " << outPhrase
      << " " << hypo.GetCurrSourceRange();
  //<< " " << hypo.m_currSourceWordsRange

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

