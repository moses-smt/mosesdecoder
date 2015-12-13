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

#ifndef moses_BitmapContainer_h
#define moses_BitmapContainer_h

#include <queue>
#include <set>
#include <vector>

#include "Hypothesis.h"
#include "HypothesisStackCubePruning.h"
#include "SquareMatrix.h"
#include "TranslationOption.h"
#include "TypeDef.h"
#include "Bitmap.h"

#include <boost/unordered_set.hpp>

namespace Moses
{

class BitmapContainer;
class BackwardsEdge;
class Hypothesis;
class HypothesisStackCubePruning;
class HypothesisQueueItem;
class QueueItemOrderer;
class TranslationOptionList;

typedef std::vector< Hypothesis* > HypothesisSet;
typedef std::set< BackwardsEdge* > BackwardsEdgeSet;
typedef std::priority_queue< HypothesisQueueItem*, std::vector< HypothesisQueueItem* >, QueueItemOrderer> HypothesisQueue;

////////////////////////////////////////////////////////////////////////////////
// Hypothesis Priority Queue Code
////////////////////////////////////////////////////////////////////////////////

//! 1 item in the priority queue for stack decoding (phrase-based)
class HypothesisQueueItem
{
private:
  size_t m_hypothesis_pos, m_translation_pos;
  Hypothesis *m_hypothesis;
  BackwardsEdge *m_edge;
  boost::shared_ptr<TargetPhrase> m_target_phrase;

  HypothesisQueueItem();

public:
  HypothesisQueueItem(const size_t hypothesis_pos
                      , const size_t translation_pos
                      , Hypothesis *hypothesis
                      , BackwardsEdge *edge
                      , const TargetPhrase *target_phrase = NULL)
    : m_hypothesis_pos(hypothesis_pos)
    , m_translation_pos(translation_pos)
    , m_hypothesis(hypothesis)
    , m_edge(edge) {
    if (target_phrase != NULL) {
      m_target_phrase.reset(new TargetPhrase(*target_phrase));
    }
  }

  ~HypothesisQueueItem() {
  }

  int GetHypothesisPos() {
    return m_hypothesis_pos;
  }

  int GetTranslationPos() {
    return m_translation_pos;
  }

  Hypothesis *GetHypothesis() {
    return m_hypothesis;
  }

  BackwardsEdge *GetBackwardsEdge() {
    return m_edge;
  }

  boost::shared_ptr<TargetPhrase> GetTargetPhrase() {
    return m_target_phrase;
  }
};

//! Allows comparison of two HypothesisQueueItem objects by the corresponding scores.
class QueueItemOrderer
{
public:
  bool operator()(HypothesisQueueItem* itemA, HypothesisQueueItem* itemB) const {
    float scoreA = itemA->GetHypothesis()->GetFutureScore();
    float scoreB = itemB->GetHypothesis()->GetFutureScore();

    if (scoreA < scoreB) {
      return true;
    } else if (scoreA > scoreB) {
      return false;
    } else {
      // Equal scores: break ties by comparing target phrases (if they exist)
      // *Important*: these are pointers to copies of the target phrases from the
      // hypotheses.  This class is used to keep priority queues ordered in the
      // background, so comparisons made as those data structures are cleaned up
      // may occur *after* the target phrases in hypotheses have been cleaned up,
      // leading to segfaults if relying on hypotheses to provide target phrases.
      boost::shared_ptr<TargetPhrase> phrA = itemA->GetTargetPhrase();
      boost::shared_ptr<TargetPhrase> phrB = itemB->GetTargetPhrase();
      if (!phrA || !phrB) {
        // Fallback: scoreA < scoreB == false, non-deterministic sort
        return false;
      }
      return (phrA->Compare(*phrB) > 0);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// Hypothesis Orderer Code
////////////////////////////////////////////////////////////////////////////////
// Allows to compare two Hypothesis objects by the corresponding scores.
////////////////////////////////////////////////////////////////////////////////

class HypothesisScoreOrderer
{
private:
  bool m_deterministic;

public:
  HypothesisScoreOrderer(const bool deterministic = false)
    : m_deterministic(deterministic) {}

  bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const {

    float scoreA = hypoA->GetFutureScore();
    float scoreB = hypoB->GetFutureScore();

    if (scoreA > scoreB) {
      return true;
    } else if (scoreA < scoreB) {
      return false;
    } else {
      if (m_deterministic) {
        // Equal scores: break ties by comparing target phrases
        return (hypoA->GetCurrTargetPhrase().Compare(hypoB->GetCurrTargetPhrase()) < 0);
      }
      // Fallback: scoreA > scoreB == false, non-deterministic sort
      return false;
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// Backwards Edge Code
////////////////////////////////////////////////////////////////////////////////
// Encodes an edge pointing to a BitmapContainer.
////////////////////////////////////////////////////////////////////////////////

class BackwardsEdge
{
private:
  friend class BitmapContainer;
  bool m_initialized;

  const BitmapContainer &m_prevBitmapContainer;
  BitmapContainer &m_parent;
  const TranslationOptionList &m_translations;
  const SquareMatrix &m_estimatedScores;
  float m_estimatedScore;

  bool m_deterministic;

  std::vector< const Hypothesis* > m_hypotheses;
  boost::unordered_set< int > m_seenPosition;

  // We don't want to instantiate "empty" objects.
  BackwardsEdge();

  Hypothesis *CreateHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt);
  bool SeenPosition(const size_t x, const size_t y);
  void SetSeenPosition(const size_t x, const size_t y);

protected:
  void Initialize();

public:
  BackwardsEdge(const BitmapContainer &prevBitmapContainer
                , BitmapContainer &parent
                , const TranslationOptionList &translations
                , const SquareMatrix &estimatedScores
                , const InputType& source
                , const bool deterministic = false);
  ~BackwardsEdge();

  bool GetInitialized();
  const BitmapContainer &GetBitmapContainer() const;
  int GetDistortionPenalty();
  void PushSuccessors(const size_t x, const size_t y);
};

////////////////////////////////////////////////////////////////////////////////
// Bitmap Container Code
////////////////////////////////////////////////////////////////////////////////
// A BitmapContainer encodes an ordered set of hypotheses and a set of edges
// pointing to the "generating" BitmapContainers.  It also stores a priority
// queue that contains expanded hypotheses from the connected edges.
////////////////////////////////////////////////////////////////////////////////

class BitmapContainer
{
private:
  const Bitmap &m_bitmap;
  HypothesisStackCubePruning &m_stack;
  HypothesisSet m_hypotheses;
  BackwardsEdgeSet m_edges;
  HypothesisQueue m_queue;
  size_t m_numStackInsertions;
  bool m_deterministic;

  // We always require a corresponding bitmap to be supplied.
  BitmapContainer();
  BitmapContainer(const BitmapContainer &);
public:
  BitmapContainer(const Bitmap &bitmap
                  , HypothesisStackCubePruning &stack
                  , bool deterministic = false);

  // The destructor will also delete all the edges that are
  // connected to this BitmapContainer.
  ~BitmapContainer();

  void Enqueue(int hypothesis_pos, int translation_pos, Hypothesis *hypothesis, BackwardsEdge *edge);
  HypothesisQueueItem *Dequeue(bool keepValue=false);
  HypothesisQueueItem *Top() const;
  size_t Size();
  bool Empty() const;

  const Bitmap &GetWordsBitmap() const {
    return m_bitmap;
  }

  const HypothesisSet &GetHypotheses() const;
  size_t GetHypothesesSize() const;
  const BackwardsEdgeSet &GetBackwardsEdges();

  void InitializeEdges();
  void ProcessBestHypothesis();
  void EnsureMinStackHyps(const size_t minNumHyps);
  void AddHypothesis(Hypothesis *hypothesis);
  void AddBackwardsEdge(BackwardsEdge *edge);
  void SortHypotheses();
};

}

#endif
