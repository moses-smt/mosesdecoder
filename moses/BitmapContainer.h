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
#include "WordsBitmap.h"

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

  HypothesisQueueItem();

public:
  HypothesisQueueItem(const size_t hypothesis_pos
                      , const size_t translation_pos
                      , Hypothesis *hypothesis
                      , BackwardsEdge *edge)
    : m_hypothesis_pos(hypothesis_pos)
    , m_translation_pos(translation_pos)
    , m_hypothesis(hypothesis)
    , m_edge(edge) {
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
};

//! Allows comparison of two HypothesisQueueItem objects by the corresponding scores.
class QueueItemOrderer
{
public:
  bool operator()(HypothesisQueueItem* itemA, HypothesisQueueItem* itemB) const {
    float scoreA = itemA->GetHypothesis()->GetTotalScore();
    float scoreB = itemB->GetHypothesis()->GetTotalScore();

    return (scoreA < scoreB);

    /*
    {
    	return true;
    }
    else if (scoreA < scoreB)
    {
    	return false;
    }
    else
    {
    	return itemA < itemB;
    }*/
  }
};

////////////////////////////////////////////////////////////////////////////////
// Hypothesis Orderer Code
////////////////////////////////////////////////////////////////////////////////
// Allows to compare two Hypothesis objects by the corresponding scores.
////////////////////////////////////////////////////////////////////////////////

class HypothesisScoreOrderer
{
public:
  bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const {
    float scoreA = hypoA->GetTotalScore();
    float scoreB = hypoB->GetTotalScore();

    return (scoreA > scoreB);
    /*
    {
    	return true;
    }
    else if (scoreA < scoreB)
    	{
    		return false;
    	}
    else
    	{
    		return hypoA < hypoB;
    	}*/
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
  const SquareMatrix &m_futurescore;

  std::vector< const Hypothesis* > m_hypotheses;
  std::set< int > m_seenPosition;

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
                , const SquareMatrix &futureScore,
                const InputType& source);
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
  WordsBitmap m_bitmap;
  HypothesisStackCubePruning &m_stack;
  HypothesisSet m_hypotheses;
  BackwardsEdgeSet m_edges;
  HypothesisQueue m_queue;
  size_t m_numStackInsertions;

  // We always require a corresponding bitmap to be supplied.
  BitmapContainer();
  BitmapContainer(const BitmapContainer &);
public:
  BitmapContainer(const WordsBitmap &bitmap
                  , HypothesisStackCubePruning &stack);

  // The destructor will also delete all the edges that are
  // connected to this BitmapContainer.
  ~BitmapContainer();

  void Enqueue(int hypothesis_pos, int translation_pos, Hypothesis *hypothesis, BackwardsEdge *edge);
  HypothesisQueueItem *Dequeue(bool keepValue=false);
  HypothesisQueueItem *Top() const;
  size_t Size();
  bool Empty() const;

  const WordsBitmap &GetWordsBitmap();
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
