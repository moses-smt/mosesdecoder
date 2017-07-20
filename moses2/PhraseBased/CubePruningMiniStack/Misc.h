/*
 * CubePruning.h
 *
 *  Created on: 27 Nov 2015
 *      Author: hieu
 */
#pragma once
#include <boost/pool/pool_alloc.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <vector>
#include <queue>
#include "../../legacy/Range.h"
#include "../Hypothesis.h"
#include "../../TypeDef.h"
#include "../../Vector.h"
#include "../../MemPoolAllocator.h"
#include "Stack.h"

namespace Moses2
{

class Manager;
class InputPath;
class TargetPhrases;
class Bitmap;

namespace NSCubePruningMiniStack
{
class CubeEdge;

class QueueItem;
typedef std::deque<QueueItem*, MemPoolAllocator<QueueItem*> > QueueItemRecycler;

///////////////////////////////////////////
class QueueItem
{
  ~QueueItem(); // NOT IMPLEMENTED. Use MemPool
public:
  static QueueItem *Create(QueueItem *currItem, Manager &mgr, CubeEdge &edge,
                           size_t hypoIndex, size_t tpIndex,
                           QueueItemRecycler &queueItemRecycler);
  QueueItem(Manager &mgr, CubeEdge &edge, size_t hypoIndex, size_t tpIndex);

  void Init(Manager &mgr, CubeEdge &edge, size_t hypoIndex, size_t tpIndex);

  CubeEdge *edge;
  size_t hypoIndex, tpIndex;
  Hypothesis *hypo;

protected:
  void CreateHypothesis(Manager &mgr);
};

///////////////////////////////////////////
class QueueItemOrderer
{
public:
  bool operator()(QueueItem* itemA, QueueItem* itemB) const {
    HypothesisFutureScoreOrderer orderer;
    return !orderer(itemA->hypo, itemB->hypo);
  }
};

///////////////////////////////////////////
class CubeEdge
{
public:
  typedef std::priority_queue<QueueItem*,
          std::vector<QueueItem*, MemPoolAllocator<QueueItem*> >, QueueItemOrderer> Queue;

  typedef std::pair<const CubeEdge*, int> SeenPositionItem;
  typedef boost::unordered_set<SeenPositionItem, boost::hash<SeenPositionItem>,
          std::equal_to<SeenPositionItem>, MemPoolAllocator<SeenPositionItem> > SeenPositions;

  const Hypotheses &hypos;
  const InputPath &path;
  const TargetPhrases &tps;
  const Bitmap &newBitmap;
  SCORE estimatedScore;

  CubeEdge(Manager &mgr, const Hypotheses &hypos, const InputPath &path,
           const TargetPhrases &tps, const Bitmap &newBitmap);

  bool SetSeenPosition(const size_t x, const size_t y,
                       SeenPositions &seenPositions) const;

  void CreateFirst(Manager &mgr, Queue &queue, SeenPositions &seenPositions,
                   QueueItemRecycler &queueItemRecycler);
  void CreateNext(Manager &mgr, QueueItem *item, Queue &queue,
                  SeenPositions &seenPositions,
                  QueueItemRecycler &queueItemRecycler);

  std::string Debug(const System &system) const;

protected:

};

}

}

