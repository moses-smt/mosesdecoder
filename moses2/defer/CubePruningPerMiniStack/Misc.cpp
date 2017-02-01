/*
 * CubePruning.cpp
 *
 *  Created on: 27 Nov 2015
 *      Author: hieu
 */

#include "Misc.h"
#include "../Manager.h"
#include "../../MemPool.h"
#include "../../System.h"

using namespace std;

namespace Moses2
{

namespace NSCubePruningPerMiniStack
{

////////////////////////////////////////////////////////////////////////
QueueItem *QueueItem::Create(QueueItem *currItem,
                             Manager &mgr,
                             CubeEdge &edge,
                             size_t hypoIndex,
                             size_t tpIndex,
                             std::deque<QueueItem*> &queueItemRecycler)
{
  QueueItem *ret;
  if (currItem) {
    // reuse incoming queue item to create new item
    ret = currItem;
    ret->Init(mgr, edge, hypoIndex, tpIndex);
  } else if (!queueItemRecycler.empty()) {
    // use item from recycle bin
    ret = queueItemRecycler.back();
    ret->Init(mgr, edge, hypoIndex, tpIndex);
    queueItemRecycler.pop_back();
  } else {
    // create new item
    ret = new (mgr.GetPool().Allocate<QueueItem>()) QueueItem(mgr, edge, hypoIndex, tpIndex);
  }

  return ret;
}

QueueItem::QueueItem(Manager &mgr, CubeEdge &edge, size_t hypoIndex, size_t tpIndex)
  :edge(&edge)
  ,hypoIndex(hypoIndex)
  ,tpIndex(tpIndex)
{
  CreateHypothesis(mgr);
}

void QueueItem::Init(Manager &mgr, CubeEdge &edge, size_t hypoIndex, size_t tpIndex)
{
  this->edge = &edge;
  this->hypoIndex = hypoIndex;
  this->tpIndex = tpIndex;

  CreateHypothesis(mgr);
}

void QueueItem::CreateHypothesis(Manager &mgr)
{
  const Hypothesis *prevHypo = edge->miniStack.GetSortedAndPruneHypos(mgr)[hypoIndex];
  const TargetPhrase &tp = edge->tps[tpIndex];

  //cerr << "hypoIndex=" << hypoIndex << endl;
  //cerr << "edge.hypos=" << edge.hypos.size() << endl;
  //cerr << prevHypo << endl;
  //cerr << *prevHypo << endl;

  hypo = Hypothesis::Create(mgr.GetSystemPool(), mgr);
  hypo->Init(mgr, *prevHypo, edge->path, tp, edge->newBitmap, edge->estimatedScore);
  hypo->EvaluateWhenApplied();
}

////////////////////////////////////////////////////////////////////////
CubeEdge::CubeEdge(
  Manager &mgr,
  const NSCubePruningMiniStack::MiniStack &miniStack,
  const InputPath &path,
  const TargetPhrases &tps,
  const Bitmap &newBitmap)
  :miniStack(miniStack)
  ,path(path)
  ,tps(tps)
  ,newBitmap(newBitmap)
{
  estimatedScore = mgr.GetEstimatedScores().CalcEstimatedScore(newBitmap);
}

std::ostream& operator<<(std::ostream &out, const CubeEdge &obj)
{
  out << obj.newBitmap;
  return out;
}

bool
CubeEdge::SetSeenPosition(const size_t x, const size_t y, SeenPositions &seenPositions) const
{
  //UTIL_THROW_IF2(x >= (1<<17), "Error");
  //UTIL_THROW_IF2(y >= (1<<17), "Error");

  SeenPositionItem val(this, (x<<16) + y);
  std::pair<SeenPositions::iterator, bool> pairRet = seenPositions.insert(val);
  return pairRet.second;
}

void CubeEdge::CreateFirst(Manager &mgr,
                           Queue &queue,
                           SeenPositions &seenPositions,
                           std::deque<QueueItem*> &queueItemRecycler)
{
  if (miniStack.GetSortedAndPruneHypos(mgr).size()) {
    assert(tps.GetSize());

    QueueItem *item = QueueItem::Create(NULL, mgr, *this, 0, 0, queueItemRecycler);
    queue.push(item);
    bool setSeen = SetSeenPosition(0, 0, seenPositions);
    assert(setSeen);
  }
}

void CubeEdge::CreateNext(Manager &mgr,
                          QueueItem *item,
                          Queue &queue,
                          SeenPositions &seenPositions,
                          std::deque<QueueItem*> &queueItemRecycler)
{
  size_t hypoIndex = item->hypoIndex;
  size_t tpIndex = item->tpIndex;

  if (hypoIndex + 1 < miniStack.GetSortedAndPruneHypos(mgr).size() && SetSeenPosition(hypoIndex + 1, tpIndex, seenPositions)) {
    // reuse incoming queue item to create new item
    QueueItem *newItem = QueueItem::Create(item, mgr, *this, hypoIndex + 1, tpIndex, queueItemRecycler);
    assert(newItem == item);
    queue.push(newItem);
    item = NULL;
  }

  if (tpIndex + 1 < tps.GetSize() && SetSeenPosition(hypoIndex, tpIndex + 1, seenPositions)) {
    QueueItem *newItem = QueueItem::Create(item, mgr, *this, hypoIndex, tpIndex + 1, queueItemRecycler);
    queue.push(newItem);
    item = NULL;
  }

  if (item) {
    // recycle unused queue item
    queueItemRecycler.push_back(item);
  }
}

}

}


