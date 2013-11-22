// $Id$
// vim:tabstop=2
/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>

#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "Hypothesis.h"
#include "Util.h"
#include "SquareMatrix.h"
#include "StaticData.h"
#include "InputType.h"
#include "Manager.h"
#include "moses/FF/FFState.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"

using namespace std;

namespace Moses
{

#ifdef USE_HYPO_POOL
ObjectPool<Hypothesis> Hypothesis::s_objectPool("Hypothesis", 300000);
#endif

Hypothesis::Hypothesis(Manager& manager, InputType const& source, const TranslationOption &initialTransOpt)
  : m_prevHypo(NULL)
  , m_sourceCompleted(source.GetSize(), manager.m_source.m_sourceCompleted)
  , m_sourceInput(source)
  , m_currSourceWordsRange(
    m_sourceCompleted.GetFirstGapPos()>0 ? 0 : NOT_FOUND,
    m_sourceCompleted.GetFirstGapPos()>0 ? m_sourceCompleted.GetFirstGapPos()-1 : NOT_FOUND)
  , m_currTargetWordsRange(NOT_FOUND, NOT_FOUND)
  , m_wordDeleted(false)
  , m_totalScore(0.0f)
  , m_futureScore(0.0f)
  , m_ffStates(StatefulFeatureFunction::GetStatefulFeatureFunctions().size())
  , m_arcList(NULL)
  , m_transOpt(initialTransOpt)
  , m_manager(manager)
  , m_id(m_manager.GetNextHypoId())
{
  // used for initial seeding of trans process
  // initialize scores
  //_hash_computed = false;
  //s_HypothesesCreated = 1;
  const vector<const StatefulFeatureFunction*>& ffs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i)
    m_ffStates[i] = ffs[i]->EmptyHypothesisState(source);
  m_manager.GetSentenceStats().AddCreated();
}

/***
 * continue prevHypo by appending the phrases in transOpt
 */
Hypothesis::Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt)
  : m_prevHypo(&prevHypo)
  , m_sourceCompleted				(prevHypo.m_sourceCompleted )
  , m_sourceInput						(prevHypo.m_sourceInput)
  , m_currSourceWordsRange	(transOpt.GetSourceWordsRange())
  , m_currTargetWordsRange	( prevHypo.m_currTargetWordsRange.GetEndPos() + 1
                              ,prevHypo.m_currTargetWordsRange.GetEndPos() + transOpt.GetTargetPhrase().GetSize())
  , m_wordDeleted(false)
  , m_totalScore(0.0f)
  , m_futureScore(0.0f)
  , m_scoreBreakdown(prevHypo.GetScoreBreakdown())
  , m_ffStates(prevHypo.m_ffStates.size())
  , m_arcList(NULL)
  , m_transOpt(transOpt)
  , m_manager(prevHypo.GetManager())
  , m_id(m_manager.GetNextHypoId())
{
  m_scoreBreakdown.PlusEquals(transOpt.GetScoreBreakdown());

  // assert that we are not extending our hypothesis by retranslating something
  // that this hypothesis has already translated!
  assert(!m_sourceCompleted.Overlap(m_currSourceWordsRange));

  //_hash_computed = false;
  m_sourceCompleted.SetValue(m_currSourceWordsRange.GetStartPos(), m_currSourceWordsRange.GetEndPos(), true);
  m_wordDeleted = transOpt.IsDeletionOption();
  m_manager.GetSentenceStats().AddCreated();
}

Hypothesis::~Hypothesis()
{
  for (unsigned i = 0; i < m_ffStates.size(); ++i)
    delete m_ffStates[i];

  if (m_arcList) {
    ArcList::iterator iter;
    for (iter = m_arcList->begin() ; iter != m_arcList->end() ; ++iter) {
      FREEHYPO(*iter);
    }
    m_arcList->clear();

    delete m_arcList;
    m_arcList = NULL;
  }
}

void Hypothesis::AddArc(Hypothesis *loserHypo)
{
  if (!m_arcList) {
    if (loserHypo->m_arcList) { // we don't have an arcList, but loser does
      this->m_arcList = loserHypo->m_arcList;  // take ownership, we'll delete
      loserHypo->m_arcList = 0;                // prevent a double deletion
    } else {
      this->m_arcList = new ArcList();
    }
  } else {
    if (loserHypo->m_arcList) {  // both have an arc list: merge. delete loser
      size_t my_size = m_arcList->size();
      size_t add_size = loserHypo->m_arcList->size();
      this->m_arcList->resize(my_size + add_size, 0);
      std::memcpy(&(*m_arcList)[0] + my_size, &(*loserHypo->m_arcList)[0], add_size * sizeof(Hypothesis *));
      delete loserHypo->m_arcList;
      loserHypo->m_arcList = 0;
    } else { // loserHypo doesn't have any arcs
      // DO NOTHING
    }
  }
  m_arcList->push_back(loserHypo);
}

/***
 * return the subclass of Hypothesis most appropriate to the given translation option
 */
Hypothesis* Hypothesis::CreateNext(const TranslationOption &transOpt) const
{
  return Create(*this, transOpt);
}

/***
 * return the subclass of Hypothesis most appropriate to the given translation option
 */
Hypothesis* Hypothesis::Create(const Hypothesis &prevHypo, const TranslationOption &transOpt)
{

#ifdef USE_HYPO_POOL
  Hypothesis *ptr = s_objectPool.getPtr();
  return new(ptr) Hypothesis(prevHypo, transOpt);
#else
  return new Hypothesis(prevHypo, transOpt);
#endif
}
/***
 * return the subclass of Hypothesis most appropriate to the given target phrase
 */

Hypothesis* Hypothesis::Create(Manager& manager, InputType const& m_source, const TranslationOption &initialTransOpt)
{
#ifdef USE_HYPO_POOL
  Hypothesis *ptr = s_objectPool.getPtr();
  return new(ptr) Hypothesis(manager, m_source, initialTransOpt);
#else
  return new Hypothesis(manager, m_source, initialTransOpt);
#endif
}

/** check, if two hypothesis can be recombined.
    this is actually a sorting function that allows us to
    keep an ordered list of hypotheses. This makes recombination
    much quicker.
*/
int Hypothesis::RecombineCompare(const Hypothesis &compare) const
{
  // -1 = this < compare
  // +1 = this > compare
  // 0	= this ==compare
  int comp = m_sourceCompleted.Compare(compare.m_sourceCompleted);
  if (comp != 0)
    return comp;

  for (unsigned i = 0; i < m_ffStates.size(); ++i) {
    if (m_ffStates[i] == NULL || compare.m_ffStates[i] == NULL) {
      comp = m_ffStates[i] - compare.m_ffStates[i];
    } else {
      comp = m_ffStates[i]->Compare(*compare.m_ffStates[i]);
    }
    if (comp != 0) return comp;
  }

  return 0;
}

void Hypothesis::EvaluateWith(const StatefulFeatureFunction &sfff,
                              int state_idx)
{
  const StaticData &staticData = StaticData::Instance();
  if (! staticData.IsFeatureFunctionIgnored( sfff )) {
    m_ffStates[state_idx] = sfff.Evaluate(
                              *this,
                              m_prevHypo ? m_prevHypo->m_ffStates[state_idx] : NULL,
                              &m_scoreBreakdown);
  }
}

void Hypothesis::EvaluateWith(const StatelessFeatureFunction& slff)
{
  const StaticData &staticData = StaticData::Instance();
  if (! staticData.IsFeatureFunctionIgnored( slff )) {
    slff.Evaluate(*this, &m_scoreBreakdown);
  }
}

/***
 * calculate the logarithm of our total translation score (sum up components)
 */
void Hypothesis::Evaluate(const SquareMatrix &futureScore)
{
  clock_t t=0; // used to track time

  // some stateless score producers cache their values in the translation
  // option: add these here
  // language model scores for n-grams completely contained within a target
  // phrase are also included here

  // compute values of stateless feature functions that were not
  // cached in the translation option
  const vector<const StatelessFeatureFunction*>& sfs =
    StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for (unsigned i = 0; i < sfs.size(); ++i) {
    const StatelessFeatureFunction &ff = *sfs[i];
    EvaluateWith(ff);
  }

  const vector<const StatefulFeatureFunction*>& ffs =
    StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i) {
    const StatefulFeatureFunction &ff = *ffs[i];
    const StaticData &staticData = StaticData::Instance();
    if (! staticData.IsFeatureFunctionIgnored(ff)) {
      m_ffStates[i] = ff.Evaluate(*this,
                                  m_prevHypo ? m_prevHypo->m_ffStates[i] : NULL,
                                  &m_scoreBreakdown);
    }
  }

  IFVERBOSE(2) {
    t = clock();  // track time excluding LM
  }

  // FUTURE COST
  m_futureScore = futureScore.CalcFutureScore( m_sourceCompleted );

  // TOTAL
  m_totalScore = m_scoreBreakdown.GetWeightedScore() + m_futureScore;

  IFVERBOSE(2) {
    m_manager.GetSentenceStats().AddTimeOtherScore( clock()-t );
  }
}

const Hypothesis* Hypothesis::GetPrevHypo()const
{
  return m_prevHypo;
}

/**
 * print hypothesis information for pharaoh-style logging
 */
void Hypothesis::PrintHypothesis() const
{
  if (!m_prevHypo) {
    TRACE_ERR(endl << "NULL hypo" << endl);
    return;
  }
  TRACE_ERR(endl << "creating hypothesis "<< m_id <<" from "<< m_prevHypo->m_id<<" ( ");
  int end = (int)(m_prevHypo->GetCurrTargetPhrase().GetSize()-1);
  int start = end-1;
  if ( start < 0 ) start = 0;
  if ( m_prevHypo->m_currTargetWordsRange.GetStartPos() == NOT_FOUND ) {
    TRACE_ERR( "<s> ");
  } else {
    TRACE_ERR( "... ");
  }
  if (end>=0) {
    WordsRange range(start, end);
    TRACE_ERR( m_prevHypo->GetCurrTargetPhrase().GetSubString(range) << " ");
  }
  TRACE_ERR( ")"<<endl);
  TRACE_ERR( "\tbase score "<< (m_prevHypo->m_totalScore - m_prevHypo->m_futureScore) <<endl);
  TRACE_ERR( "\tcovering "<<m_currSourceWordsRange.GetStartPos()<<"-"<<m_currSourceWordsRange.GetEndPos()
             <<": " << m_transOpt.GetInputPath().GetPhrase() << endl);

  TRACE_ERR( "\ttranslated as: "<<(Phrase&) GetCurrTargetPhrase()<<endl); // <<" => translation cost "<<m_score[ScoreType::PhraseTrans];

  if (m_wordDeleted) TRACE_ERR( "\tword deleted"<<endl);
  //	TRACE_ERR( "\tdistance: "<<GetCurrSourceWordsRange().CalcDistortion(m_prevHypo->GetCurrSourceWordsRange())); // << " => distortion cost "<<(m_score[ScoreType::Distortion]*weightDistortion)<<endl;
  //	TRACE_ERR( "\tlanguage model cost "); // <<m_score[ScoreType::LanguageModelScore]<<endl;
  //	TRACE_ERR( "\tword penalty "); // <<(m_score[ScoreType::WordPenalty]*weightWordPenalty)<<endl;
  TRACE_ERR( "\tscore "<<m_totalScore - m_futureScore<<" + future cost "<<m_futureScore<<" = "<<m_totalScore<<endl);
  TRACE_ERR(  "\tunweighted feature scores: " << m_scoreBreakdown << endl);
  //PrintLMScores();
}

void Hypothesis::CleanupArcList()
{
  // point this hypo's main hypo to itself
  SetWinningHypo(this);

  if (!m_arcList) return;

  /* keep only number of arcs we need to create all n-best paths.
   * However, may not be enough if only unique candidates are needed,
   * so we'll keep all of arc list if nedd distinct n-best list
   */
  const StaticData &staticData = StaticData::Instance();
  size_t nBestSize = staticData.GetNBestSize();
  bool distinctNBest = staticData.GetDistinctNBest() || staticData.UseMBR() || staticData.GetOutputSearchGraph() || staticData.GetOutputSearchGraphSLF() || staticData.GetOutputSearchGraphHypergraph() || staticData.UseLatticeMBR() ;

  if (!distinctNBest && m_arcList->size() > nBestSize * 5) {
    // prune arc list only if there too many arcs
	NTH_ELEMENT4(m_arcList->begin()
                , m_arcList->begin() + nBestSize - 1
                , m_arcList->end()
                , CompareHypothesisTotalScore());

    // delete bad ones
    ArcList::iterator iter;
    for (iter = m_arcList->begin() + nBestSize ; iter != m_arcList->end() ; ++iter) {
      Hypothesis *arc = *iter;
      FREEHYPO(arc);
    }
    m_arcList->erase(m_arcList->begin() + nBestSize
                     , m_arcList->end());
  }

  // set all arc's main hypo variable to this hypo
  ArcList::iterator iter = m_arcList->begin();
  for (; iter != m_arcList->end() ; ++iter) {
    Hypothesis *arc = *iter;
    arc->SetWinningHypo(this);
  }
}

const TargetPhrase &Hypothesis::GetCurrTargetPhrase() const
{
  return m_transOpt.GetTargetPhrase();
}

void Hypothesis::GetOutputPhrase(Phrase &out) const
{
  if (m_prevHypo != NULL) {
    m_prevHypo->GetOutputPhrase(out);
  }
  out.Append(GetCurrTargetPhrase());
}

TO_STRING_BODY(Hypothesis)

// friend
ostream& operator<<(ostream& out, const Hypothesis& hypo)
{
  hypo.ToStream(out);
  // words bitmap
  out << "[" << hypo.m_sourceCompleted << "] ";

  // scores
  out << " [total=" << hypo.GetTotalScore() << "]";
  out << " " << hypo.GetScoreBreakdown();

  // alignment
  out << " " << hypo.GetCurrTargetPhrase().GetAlignNonTerm();

  /*
  const Hypothesis *prevHypo = hypo.GetPrevHypo();
  if (prevHypo)
  	out << endl << *prevHypo;
  */

  return out;
}


std::string Hypothesis::GetSourcePhraseStringRep(const vector<FactorType> factorsToPrint) const
{
  return m_transOpt.GetInputPath().GetPhrase().GetStringRep(factorsToPrint);
}

std::string Hypothesis::GetTargetPhraseStringRep(const vector<FactorType> factorsToPrint) const
{
  if (!m_prevHypo) {
    return "";
  }
  return GetCurrTargetPhrase().GetStringRep(factorsToPrint);
}

std::string Hypothesis::GetSourcePhraseStringRep() const
{
  vector<FactorType> allFactors;
  for(size_t i=0; i < MAX_NUM_FACTORS; i++) {
    allFactors.push_back(i);
  }
  return GetSourcePhraseStringRep(allFactors);
}
std::string Hypothesis::GetTargetPhraseStringRep() const
{
  vector<FactorType> allFactors;
  for(size_t i=0; i < MAX_NUM_FACTORS; i++) {
    allFactors.push_back(i);
  }
  return GetTargetPhraseStringRep(allFactors);
}

}

