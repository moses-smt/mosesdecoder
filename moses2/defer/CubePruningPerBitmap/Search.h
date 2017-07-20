/*
 * Search.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#pragma once
#include <boost/pool/pool_alloc.hpp>
#include <boost/unordered_map.hpp>
#include "../Search.h"
#include "Misc.h"
#include "Stacks.h"
#include "../../legacy/Range.h"

namespace Moses2
{

class Bitmap;
class Hypothesis;
class InputPath;
class TargetPhrases;

namespace NSCubePruningMiniStack
{
class MiniStack;
}

namespace NSCubePruningPerBitmap
{

class Search : public Moses2::Search
{
public:
  Search(Manager &mgr);
  virtual ~Search();

  virtual void Decode();
  const Hypothesis *GetBestHypo() const;

protected:
  Stacks m_stacks;

  CubeEdge::Queue m_queue;
  CubeEdge::SeenPositions m_seenPositions;

  // CUBE PRUNING VARIABLES
  // setup
  typedef std::vector<CubeEdge*> CubeEdges;
  boost::unordered_map<NSCubePruningMiniStack::MiniStack*, CubeEdges*> m_cubeEdges;

  std::deque<QueueItem*> m_queueItemRecycler;

  // CUBE PRUNING
  // decoding
  void CreateSearchGraph(size_t stackInd);
  void Decode(size_t stackInd);
  void Decode(const std::vector<NSCubePruningMiniStack::MiniStack*> &miniStacks);

  void DebugCounts();
};

}

}

