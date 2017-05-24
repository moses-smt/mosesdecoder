/*
 * Search.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#pragma once
#include <boost/pool/pool_alloc.hpp>
#include "../Search.h"
#include "Misc.h"
#include "Stack.h"
#include "../../legacy/Range.h"
#include "../../MemPoolAllocator.h"

namespace Moses2
{

class Bitmap;
class Hypothesis;
class InputPath;
class TargetPhrases;
class TargetPhraseImpl;

namespace NSCubePruningMiniStack
{

class Search: public Moses2::Search
{
public:
  Search(Manager &mgr);
  virtual ~Search();

  virtual void Decode();
  const Hypothesis *GetBestHypo() const;

  void AddInitialTrellisPaths(TrellisPaths<TrellisPath> &paths) const;

protected:
  Stack m_stack;

  CubeEdge::Queue m_queue;
  CubeEdge::SeenPositions m_seenPositions;

  // CUBE PRUNING VARIABLES
  // setup
  MemPoolAllocator<CubeEdge*> m_cubeEdgeAlloc;
  typedef std::vector<CubeEdge*, MemPoolAllocator<CubeEdge*> > CubeEdges;
  std::vector<CubeEdges*> m_cubeEdges;

  QueueItemRecycler m_queueItemRecycler;

  // CUBE PRUNING
  // decoding
  void Decode(size_t stackInd);
  void PostDecode(size_t stackInd);
};

}

}

