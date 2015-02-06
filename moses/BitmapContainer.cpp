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

#include <algorithm>
#include <limits>
#include <utility>

#include "BitmapContainer.h"
#include "HypothesisStackCubePruning.h"
#include "moses/FF/DistortionScoreProducer.h"
#include "TranslationOptionList.h"
#include "Manager.h"

namespace Moses
{

class HypothesisScoreOrdererNoDistortion
{
public:
  bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const {
    const float scoreA = hypoA->GetScore();
    const float scoreB = hypoB->GetScore();

    if (scoreA > scoreB) {
      return true;
    } else if (scoreA < scoreB) {
      return false;
    } else {
      return hypoA < hypoB;
    }
  }
};

class HypothesisScoreOrdererWithDistortion
{
public:
  HypothesisScoreOrdererWithDistortion(const WordsRange* transOptRange) :
    m_transOptRange(transOptRange) {
    m_totalWeightDistortion = 0;
    const StaticData &staticData = StaticData::Instance();

    const std::vector<const DistortionScoreProducer*> &ffs = DistortionScoreProducer::GetDistortionFeatureFunctions();
    std::vector<const DistortionScoreProducer*>::const_iterator iter;
    for (iter = ffs.begin(); iter != ffs.end(); ++iter) {
      const DistortionScoreProducer *ff = *iter;

      float weight =staticData.GetAllWeights().GetScoreForProducer(ff);
      m_totalWeightDistortion += weight;
    }
  }

  const WordsRange* m_transOptRange;
  float m_totalWeightDistortion;

  bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const {
    UTIL_THROW_IF2(m_transOptRange == NULL, "Words range not set");


    const float distortionScoreA = DistortionScoreProducer::CalculateDistortionScore(
                                     *hypoA,
                                     hypoA->GetCurrSourceWordsRange(),
                                     *m_transOptRange,
                                     hypoA->GetWordsBitmap().GetFirstGapPos()
                                   );
    const float distortionScoreB = DistortionScoreProducer::CalculateDistortionScore(
                                     *hypoB,
                                     hypoB->GetCurrSourceWordsRange(),
                                     *m_transOptRange,
                                     hypoB->GetWordsBitmap().GetFirstGapPos()
                                   );


    const float scoreA = hypoA->GetScore() + distortionScoreA * m_totalWeightDistortion;
    const float scoreB = hypoB->GetScore() + distortionScoreB * m_totalWeightDistortion;


    if (scoreA > scoreB) {
      return true;
    } else if (scoreA < scoreB) {
      return false;
    } else {
      return hypoA < hypoB;
    }
  }

};

////////////////////////////////////////////////////////////////////////////////
// BackwardsEdge Code
////////////////////////////////////////////////////////////////////////////////

BackwardsEdge::BackwardsEdge(const BitmapContainer &prevBitmapContainer
                             , BitmapContainer &parent
                             , const TranslationOptionList &translations
                             , const SquareMatrix &futureScore,
                             const InputType& itype)
  : m_initialized(false)
  , m_prevBitmapContainer(prevBitmapContainer)
  , m_parent(parent)
  , m_translations(translations)
  , m_futurescore(futureScore)
  , m_seenPosition()
{

  // If either dimension is empty, we haven't got anything to do.
  if(m_prevBitmapContainer.GetHypotheses().size() == 0 || m_translations.size() == 0) {
    VERBOSE(3, "Empty cube on BackwardsEdge" << std::endl);
    return;
  }

  // Fetch the things we need for distortion cost computation.
  int maxDistortion = StaticData::Instance().GetMaxDistortion();

  if (maxDistortion == -1) {
    for (HypothesisSet::const_iterator iter = m_prevBitmapContainer.GetHypotheses().begin(); iter != m_prevBitmapContainer.GetHypotheses().end(); ++iter) {
      m_hypotheses.push_back(*iter);
    }
    return;
  }

  const WordsRange &transOptRange = translations.Get(0)->GetSourceWordsRange();

  HypothesisSet::const_iterator iterHypo = m_prevBitmapContainer.GetHypotheses().begin();
  HypothesisSet::const_iterator iterEnd = m_prevBitmapContainer.GetHypotheses().end();

  while (iterHypo != iterEnd) {
    const Hypothesis &hypo = **iterHypo;
    // Special case: If this is the first hypothesis used to seed the search,
    // it doesn't have a valid range, and we create the hypothesis, if the
    // initial position is not further into the sentence than the distortion limit.
    if (hypo.GetWordsBitmap().GetNumWordsCovered() == 0) {
      if ((int)transOptRange.GetStartPos() <= maxDistortion)
        m_hypotheses.push_back(&hypo);
    } else {
      int distortionDistance = itype.ComputeDistortionDistance(hypo.GetCurrSourceWordsRange()
                               , transOptRange);

      if (distortionDistance <= maxDistortion)
        m_hypotheses.push_back(&hypo);
    }

    ++iterHypo;
  }

  if (m_translations.size() > 1) {
    UTIL_THROW_IF2(m_translations.Get(0)->GetFutureScore() < m_translations.Get(1)->GetFutureScore(),
                   "Non-monotonic future score: "
                   << m_translations.Get(0)->GetFutureScore() << " vs. "
                   << m_translations.Get(1)->GetFutureScore());
  }

  if (m_hypotheses.size() > 1) {
    UTIL_THROW_IF2(m_hypotheses[0]->GetTotalScore() < m_hypotheses[1]->GetTotalScore(),
                   "Non-monotonic total score"
                   << m_hypotheses[0]->GetTotalScore() << " vs. "
                   << m_hypotheses[1]->GetTotalScore());
  }

  HypothesisScoreOrdererWithDistortion orderer (&transOptRange);
  std::sort(m_hypotheses.begin(), m_hypotheses.end(), orderer);

  // std::sort(m_hypotheses.begin(), m_hypotheses.end(), HypothesisScoreOrdererNoDistortion());
}

BackwardsEdge::~BackwardsEdge()
{
  m_seenPosition.clear();
  m_hypotheses.clear();
}


void
BackwardsEdge::Initialize()
{
  if(m_hypotheses.size() == 0 || m_translations.size() == 0) {
    m_initialized = true;
    return;
  }

  Hypothesis *expanded = CreateHypothesis(*m_hypotheses[0], *m_translations.Get(0));
  m_parent.Enqueue(0, 0, expanded, this);
  SetSeenPosition(0, 0);
  m_initialized = true;
}

Hypothesis *BackwardsEdge::CreateHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt)
{
  // create hypothesis and calculate all its scores
  IFVERBOSE(2) {
    hypothesis.GetManager().GetSentenceStats().StartTimeBuildHyp();
  }
  Hypothesis *newHypo = hypothesis.CreateNext(transOpt); // TODO FIXME This is absolutely broken - don't pass null here
  IFVERBOSE(2) {
    hypothesis.GetManager().GetSentenceStats().StopTimeBuildHyp();
  }
  newHypo->EvaluateWhenApplied(m_futurescore);

  return newHypo;
}

bool
BackwardsEdge::SeenPosition(const size_t x, const size_t y)
{
  boost::unordered_set< int >::iterator iter = m_seenPosition.find((x<<16) + y);
  return (iter != m_seenPosition.end());
}

void
BackwardsEdge::SetSeenPosition(const size_t x, const size_t y)
{
  UTIL_THROW_IF2(x >= (1<<17), "Error");
  UTIL_THROW_IF2(y >= (1<<17), "Error");

  m_seenPosition.insert((x<<16) + y);
}


bool
BackwardsEdge::GetInitialized()
{
  return m_initialized;
}

const BitmapContainer&
BackwardsEdge::GetBitmapContainer() const
{
  return m_prevBitmapContainer;
}

void
BackwardsEdge::PushSuccessors(const size_t x, const size_t y)
{
  Hypothesis *newHypo;

  if(y + 1 < m_translations.size() && !SeenPosition(x, y + 1)) {
    SetSeenPosition(x, y + 1);
    newHypo = CreateHypothesis(*m_hypotheses[x], *m_translations.Get(y + 1));
    if(newHypo != NULL) {
      m_parent.Enqueue(x, y + 1, newHypo, (BackwardsEdge*)this);
    }
  }

  if(x + 1 < m_hypotheses.size() && !SeenPosition(x + 1, y)) {
    SetSeenPosition(x + 1, y);
    newHypo = CreateHypothesis(*m_hypotheses[x + 1], *m_translations.Get(y));
    if(newHypo != NULL) {
      m_parent.Enqueue(x + 1, y, newHypo, (BackwardsEdge*)this);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// BitmapContainer Code
////////////////////////////////////////////////////////////////////////////////

BitmapContainer::BitmapContainer(const WordsBitmap &bitmap
                                 , HypothesisStackCubePruning &stack)
  : m_bitmap(bitmap)
  , m_stack(stack)
  , m_numStackInsertions(0)
{
  m_hypotheses = HypothesisSet();
  m_edges = BackwardsEdgeSet();
  m_queue = HypothesisQueue();
}

BitmapContainer::~BitmapContainer()
{
  // As we have created the square position objects we clean up now.

  while (!m_queue.empty()) {
    HypothesisQueueItem *item = m_queue.top();
    m_queue.pop();

    FREEHYPO( item->GetHypothesis() );
    delete item;
  }

  // Delete all edges.
  RemoveAllInColl(m_edges);

  m_hypotheses.clear();
  m_edges.clear();
}


void
BitmapContainer::Enqueue(int hypothesis_pos
                         , int translation_pos
                         , Hypothesis *hypothesis
                         , BackwardsEdge *edge)
{
  HypothesisQueueItem *item = new HypothesisQueueItem(hypothesis_pos
      , translation_pos
      , hypothesis
      , edge);
  IFVERBOSE(2) {
    item->GetHypothesis()->GetManager().GetSentenceStats().StartTimeManageCubes();
  }
  m_queue.push(item);
  IFVERBOSE(2) {
    item->GetHypothesis()->GetManager().GetSentenceStats().StopTimeManageCubes();
  }
}

HypothesisQueueItem*
BitmapContainer::Dequeue(bool keepValue)
{
  if (!m_queue.empty()) {
    HypothesisQueueItem *item = m_queue.top();

    if (!keepValue) {
      m_queue.pop();
    }

    return item;
  }

  return NULL;
}

HypothesisQueueItem*
BitmapContainer::Top() const
{
  return m_queue.top();
}

size_t
BitmapContainer::Size()
{
  return m_queue.size();
}

bool
BitmapContainer::Empty() const
{
  return m_queue.empty();
}


const WordsBitmap&
BitmapContainer::GetWordsBitmap()
{
  return m_bitmap;
}

const HypothesisSet&
BitmapContainer::GetHypotheses() const
{
  return m_hypotheses;
}

size_t
BitmapContainer::GetHypothesesSize() const
{
  return m_hypotheses.size();
}

const BackwardsEdgeSet&
BitmapContainer::GetBackwardsEdges()
{
  return m_edges;
}

void
BitmapContainer::AddHypothesis(Hypothesis *hypothesis)
{
  bool itemExists = false;
  HypothesisSet::const_iterator iter = m_hypotheses.begin();
  HypothesisSet::const_iterator iterEnd = m_hypotheses.end();

  // cfedermann: do we actually need this check?
  while (iter != iterEnd) {
    if (*iter == hypothesis) {
      itemExists = true;
      break;
    }

    ++iter;
  }
  UTIL_THROW_IF2(itemExists, "Duplicate hypotheses");
  m_hypotheses.push_back(hypothesis);
}

void
BitmapContainer::AddBackwardsEdge(BackwardsEdge *edge)
{
  m_edges.insert(edge);
}

void
BitmapContainer::InitializeEdges()
{
  BackwardsEdgeSet::iterator iter = m_edges.begin();
  BackwardsEdgeSet::iterator iterEnd = m_edges.end();

  while (iter != iterEnd) {
    BackwardsEdge *edge = *iter;
    edge->Initialize();

    ++iter;
  }
}

void
BitmapContainer::EnsureMinStackHyps(const size_t minNumHyps)
{
  while ((!Empty()) && m_numStackInsertions < minNumHyps) {
    ProcessBestHypothesis();
  }
}

void
BitmapContainer::ProcessBestHypothesis()
{
  if (m_queue.empty()) {
    return;
  }

  // Get the currently best hypothesis from the queue.
  HypothesisQueueItem *item = Dequeue();

  // If the priority queue is exhausted, we are done and should have exited
  UTIL_THROW_IF2(item == NULL, "Null object");

  // check we are pulling things off of priority queue in right order
  if (!Empty()) {
    HypothesisQueueItem *check = Dequeue(true);
    UTIL_THROW_IF2(item->GetHypothesis()->GetTotalScore() < check->GetHypothesis()->GetTotalScore(),
                   "Non-monotonic total score: "
                   << item->GetHypothesis()->GetTotalScore() << " vs. "
                   << check->GetHypothesis()->GetTotalScore());
  }

  // Logging for the criminally insane
  IFVERBOSE(3) {
    item->GetHypothesis()->PrintHypothesis();
  }

  // Add best hypothesis to hypothesis stack.
  const bool newstackentry = m_stack.AddPrune(item->GetHypothesis());
  if (newstackentry)
    m_numStackInsertions++;

  IFVERBOSE(3) {
    TRACE_ERR("new stack entry flag is " << newstackentry << std::endl);
  }

  // Create new hypotheses for the two successors of the hypothesis just added.
  item->GetBackwardsEdge()->PushSuccessors(item->GetHypothesisPos(), item->GetTranslationPos());

  // We are done with the queue item, we delete it.
  delete item;
}

void
BitmapContainer::SortHypotheses()
{
  std::sort(m_hypotheses.begin(), m_hypotheses.end(), HypothesisScoreOrderer());
}

}

