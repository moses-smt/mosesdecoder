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

namespace NSCubePruning
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
	const Hypothesis *GetBestHypothesis() const;

protected:
	Stacks m_stacks;

	CubeEdge::Queue m_queue;
	CubeEdge::SeenPositions m_seenPositions;

	// CUBE PRUNING VARIABLES
	// setup
	MemPoolAllocator<CubeEdge*> m_cubeEdgeAlloc;
	typedef std::vector<CubeEdge*, MemPoolAllocator<CubeEdge*> > CubeEdges;
	boost::unordered_map<NSCubePruning::MiniStack*, CubeEdges*> m_cubeEdges;

	std::deque<QueueItem*> m_queueItemRecycler;

	// CUBE PRUNING
	// decoding
	void CreateSearchGraph(size_t stackInd);
	void Decode(size_t stackInd);
	void Decode(NSCubePruning::MiniStack &miniStack);

	void DebugCounts();
};

}

}

