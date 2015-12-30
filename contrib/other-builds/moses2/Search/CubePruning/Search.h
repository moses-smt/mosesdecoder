/*
 * Search.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#pragma once

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

class Search : public Moses2::Search
{
public:
	Search(Manager &mgr);
	virtual ~Search();

	virtual void Decode();
	const Hypothesis *GetBestHypothesis() const;

protected:
	Stacks m_stacks;

	QueueItemOrderer m_queueOrder;
	MemPoolAllocator<QueueItem*> m_queueContainerAlloc;
	std::vector<QueueItem*, MemPoolAllocator<QueueItem*> > m_queueContainer;
	CubeEdge::Queue m_queue;

	MemPoolAllocator<CubeEdge::SeenPositionItem> m_seenPositionsAlloc;
	CubeEdge::SeenPositions m_seenPositions;

	// CUBE PRUNING VARIABLES
	// setup
	typedef std::vector<CubeEdge*> CubeEdges;
	std::vector<CubeEdges> m_cubeEdges;

	// CUBE PRUNING
	// decoding
	void Decode(size_t stackInd);
	void PostDecode(size_t stackInd);
	void Prefetch(size_t stackInd);
};

}

}

