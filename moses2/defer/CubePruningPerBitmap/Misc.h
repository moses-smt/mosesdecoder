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
#include "../CubePruningMiniStack/Stack.h"

namespace Moses2
{

class Manager;
class InputPath;
class TargetPhrases;
class Bitmap;

namespace NSCubePruningPerBitmap
{
class CubeEdge;

///////////////////////////////////////////
class QueueItem
{
  ~QueueItem(); // NOT IMPLEMENTED. Use MemPool
public:
  static QueueItem *Create(QueueItem *currItem,
                           Manager &mgr,
                           CubeEdge &edge,
                           size_t hypoIndex,
                           size_t tpIndex,
                           std::deque<QueueItem*> &queueItemRecycler);
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
  friend std::ostream& operator<<(std::ostream &, const CubeEdge &);

public:
  typedef std::priority_queue<QueueItem*,
          std::vector<QueueItem*>,
          QueueItemOrderer> Queue;

  typedef std::pair<const CubeEdge*, int> SeenPositionItem;
  typedef boost::unordered_set<SeenPositionItem,
          boost::hash<SeenPositionItem>,
          std::equal_to<SeenPositionItem>
          > SeenPositions;

  const NSCubePruningMiniStack::MiniStack &miniStack;
  const InputPath &path;
  const TargetPhrases &tps;
  const Bitmap &newBitmap;
  SCORE estimatedScore;

  CubeEdge(Manager &mgr,
           const NSCubePruningMiniStack::MiniStack &miniStack,
           const InputPath &path,
           const TargetPhrases &tps,
           const Bitmap &newBitmap);

  bool SetSeenPosition(const size_t x, const size_t y, SeenPositions &seenPositions) const;

  void CreateFirst(Manager &mgr,
                   Queue &queue,
                   SeenPositions &seenPositions,
                   std::deque<QueueItem*> &queueItemRecycler);
  void CreateNext(Manager &mgr,
                  QueueItem *item,
                  Queue &queue,
                  SeenPositions &seenPositions,
                  std::deque<QueueItem*> &queueItemRecycler);


protected:

};

}

}


