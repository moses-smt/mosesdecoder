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

#pragma once

#include <vector>
#include <boost/scoped_ptr.hpp>
#include "Util.h"
#include "Range.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"
#include "ChartTranslationOptions.h"
#include "ObjectPool.h"

namespace Moses
{

class ChartKBestExtractor;
class ChartHypothesis;
class ChartManager;
class RuleCubeItem;
class FFState;

typedef std::vector<ChartHypothesis*> ChartArcList;

/** a hypothesis in the hierarchical/syntax decoder.
 * Contain a pointer to the current target phrase, a vector of previous hypos, and some scores
 */
class ChartHypothesis
{
  friend std::ostream& operator<<(std::ostream&, const ChartHypothesis&);
//  friend class ChartKBestExtractor;

protected:

  boost::shared_ptr<ChartTranslationOption> m_transOpt;

  Range m_currSourceWordsRange;
  std::vector<const FFState*> m_ffStates; /*! stateful feature function states */
  /*! sum of scores of this hypothesis, and previous hypotheses. Lazily initialised.  */
  mutable boost::scoped_ptr<ScoreComponentCollection> m_scoreBreakdown;
  mutable boost::scoped_ptr<ScoreComponentCollection> m_deltaScoreBreakdown;
  ScoreComponentCollection m_currScoreBreakdown /*! scores for this hypothesis only */
  ,m_lmNGram
  ,m_lmPrefix;
  float m_totalScore;

  ChartArcList *m_arcList; /*! all arcs that end at the same trellis point as this hypothesis */
  const ChartHypothesis *m_winningHypo;

  std::vector<const ChartHypothesis*> m_prevHypos; // always sorted by source position?

  ChartManager& m_manager;

  unsigned m_id; /* pkoehn wants to log the order in which hypotheses were generated */

  //! not implemented
  ChartHypothesis();

  //! not implemented
  ChartHypothesis(const ChartHypothesis &copy);

public:
  ChartHypothesis(const ChartTranslationOptions &, const RuleCubeItem &item,
                  ChartManager &manager);

  //! only used by ChartKBestExtractor
  ChartHypothesis(const ChartHypothesis &, const ChartKBestExtractor &);

  ~ChartHypothesis();

  unsigned GetId() const {
    return m_id;
  }

  const ChartTranslationOption &GetTranslationOption() const {
    return *m_transOpt;
  }

  //! Get the rule that created this hypothesis
  const TargetPhrase &GetCurrTargetPhrase() const {
    return m_transOpt->GetPhrase();
  }

  //! the source range that this hypothesis spans
  const Range &GetCurrSourceRange() const {
    return m_currSourceWordsRange;
  }

  //! the arc list when creating n-best lists
  inline const ChartArcList* GetArcList() const {
    return m_arcList;
  }

  //! the feature function states for a particular feature \param featureID
  inline const FFState* GetFFState( size_t featureID ) const {
    return m_ffStates[ featureID ];
  }

  //! reference back to the manager
  inline const ChartManager& GetManager() const {
    return m_manager;
  }

  void GetOutputPhrase(Phrase &outPhrase) const;
  Phrase GetOutputPhrase() const;

  // get leftmost/rightmost words only
  // leftRightMost: 1=left, 2=right
  void GetOutputPhrase(size_t leftRightMost, size_t numWords, Phrase &outPhrase) const;

  void EvaluateWhenApplied();

  void AddArc(ChartHypothesis *loserHypo);
  void CleanupArcList();
  void SetWinningHypo(const ChartHypothesis *hypo);

  //! get the unweighted score for each feature function
  const ScoreComponentCollection &GetScoreBreakdown() const {
    // Note: never call this method before m_currScoreBreakdown is fully computed
    if (!m_scoreBreakdown.get()) {
      m_scoreBreakdown.reset(new ScoreComponentCollection());
      // score breakdown from current translation rule
      if (m_transOpt) {
        m_scoreBreakdown->PlusEquals(GetTranslationOption().GetScores());
      }
      m_scoreBreakdown->PlusEquals(m_currScoreBreakdown);
      // score breakdowns from prev hypos
      for (std::vector<const ChartHypothesis*>::const_iterator iter = m_prevHypos.begin(); iter != m_prevHypos.end(); ++iter) {
        const ChartHypothesis &prevHypo = **iter;
        m_scoreBreakdown->PlusEquals(prevHypo.GetScoreBreakdown());
      }
    }
    return *(m_scoreBreakdown.get());
  }

  //! get the unweighted score delta for each feature function
  const ScoreComponentCollection &GetDeltaScoreBreakdown() const {
    // Note: never call this method before m_currScoreBreakdown is fully computed
    if (!m_deltaScoreBreakdown.get()) {
      m_deltaScoreBreakdown.reset(new ScoreComponentCollection());
      // score breakdown from current translation rule
      if (m_transOpt) {
        m_deltaScoreBreakdown->PlusEquals(GetTranslationOption().GetScores());
      }
      m_deltaScoreBreakdown->PlusEquals(m_currScoreBreakdown);
      // delta: score breakdowns from prev hypos _not_ added
    }
    return *(m_deltaScoreBreakdown.get());
  }

  //! Get the weighted total score
  float GetFutureScore() const {
    // scores from current translation rule. eg. translation models & word penalty
    return m_totalScore;
  }

  //! vector of previous hypotheses this hypo is built on
  const std::vector<const ChartHypothesis*> &GetPrevHypos() const {
    return m_prevHypos;
  }

  //! get a particular previous hypos
  const ChartHypothesis* GetPrevHypo(size_t pos) const {
    return m_prevHypos[pos];
  }

  //! get the constituency label that covers this hypo
  const Word &GetTargetLHS() const {
    return GetCurrTargetPhrase().GetTargetLHS();
  }

  //! get the best hypo in the arc list when doing n-best list creation. It's either this hypothesis, or the best hypo is this hypo is in the arc list
  const ChartHypothesis* GetWinningHypothesis() const {
    return m_winningHypo;
  }

  // for unordered_set in stack
  size_t hash() const;
  bool operator==(const ChartHypothesis& other) const;

  TO_STRING();

}; // class ChartHypothesis

}

