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

#pragma once

#include <vector>
#include "Util.h"
#include "WordsRange.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"
#include "ChartTranslationOption.h"
#include "ObjectPool.h"

namespace Moses
{
class QueueEntry;
class ChartHypothesis;
class ChartManager;

typedef std::vector<ChartHypothesis*> ChartArcList;

class ChartHypothesis
{
  friend std::ostream& operator<<(std::ostream&, const ChartHypothesis&);

protected:

#ifdef USE_HYPO_POOL
  static ObjectPool<ChartHypothesis> s_objectPool;
#endif

  static unsigned int s_HypothesesCreated;

  int m_id; /**< numeric ID of this hypothesis, used for logging */
  const Moses::ChartTranslationOption &m_transOpt;

  Moses::Phrase m_contextPrefix, m_contextSuffix;
  const std::vector<size_t> &m_wordsConsumedTargetOrder; // same size as target phrase ?
  Moses::WordsRange					m_currSourceWordsRange;
  Moses::ScoreComponentCollection m_scoreBreakdown /*! detailed score break-down by components (for instance language model, word penalty, etc) */
  ,m_lmNGram
  ,m_lmPrefix;
  float m_totalScore;
  size_t m_numTargetTerminals;

  ChartArcList 					*m_arcList; /*! all arcs that end at the same trellis point as this hypothesis */
  const ChartHypothesis 	*m_winningHypo;

  std::vector<const ChartHypothesis*> m_prevHypos;

  ChartManager& m_manager;

  size_t CalcPrefix(Moses::Phrase &ret, size_t size) const;
  size_t CalcSuffix(Moses::Phrase &ret, size_t size) const;

  void CalcLMScore();

  ChartHypothesis(); // not implemented
  ChartHypothesis(const ChartHypothesis &copy); // not implemented

public:
  static void ResetHypoCount() {
    s_HypothesesCreated = 0;
  }
  static unsigned int GetHypoCount() {
    return s_HypothesesCreated;
  }

#ifdef USE_HYPO_POOL
  void *operator new(size_t /* num_bytes */) {
    void *ptr = s_objectPool.getPtr();
    return ptr;
  }

  static void Delete(ChartHypothesis *hypo) {
    s_objectPool.freeObject(hypo);
  }
#else
  static void Delete(ChartHypothesis *hypo) {
    delete hypo;
  }
#endif

  explicit ChartHypothesis(const QueueEntry &queueEntry, ChartManager &manager);
  ~ChartHypothesis();

  int GetId()const {
    return m_id;
  }
  const Moses::ChartTranslationOption &GetTranslationOption()const {
    return m_transOpt;
  }
  const Moses::TargetPhrase &GetCurrTargetPhrase()const {
    return m_transOpt.GetTargetPhrase();
  }
  const Moses::WordsRange &GetCurrSourceRange()const {
    return m_currSourceWordsRange;
  }
  inline const ChartArcList* GetArcList() const {
    return m_arcList;
  }

  void CreateOutputPhrase(Moses::Phrase &outPhrase) const;
  Moses::Phrase GetOutputPhrase() const;

  int LMContextCompare(const ChartHypothesis &other) const;

  const Moses::Phrase &GetPrefix() const {
    return m_contextPrefix;
  }
  const Moses::Phrase &GetSuffix() const {
    return m_contextSuffix;
  }

  void CalcScore();

  void AddArc(ChartHypothesis *loserHypo);
  void CleanupArcList();
  void SetWinningHypo(const ChartHypothesis *hypo);

  const Moses::ScoreComponentCollection &GetScoreBreakdown() const {
    return m_scoreBreakdown;
  }
  float GetTotalScore() const {
    return m_totalScore;
  }

  const std::vector<const ChartHypothesis*> &GetPrevHypos() const {
    return m_prevHypos;
  }

  size_t GetWordsConsumedTargetOrder(size_t pos) const {
    assert(pos < m_wordsConsumedTargetOrder.size());
    return m_wordsConsumedTargetOrder[pos];
  }

  const Moses::Word &GetTargetLHS() const {
    return GetCurrTargetPhrase().GetTargetLHS();
  }

  size_t GetNumTargetTerminals() const {
    return m_numTargetTerminals;
  }

  TO_STRING();

}; // class ChartHypothesis

}

