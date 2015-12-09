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

class Bitmap;
class Hypothesis;
class InputPath;
class TargetPhrases;

namespace NSCubePruning
{

class Search : public ::Search
{
public:
	Search(Manager &mgr);
	virtual ~Search();

	virtual void Decode();
	const Hypothesis *GetBestHypothesis() const;

protected:
	NSCubePruning::Stacks m_stacks;
	NSCubePruning::CubeEdge::Queue &m_queue;

	// CUBE PRUNING VARIABLES
	// setup
	typedef std::list<CubeEdge*> CubeEdges;
	std::vector<CubeEdges> m_cubeEdges;

	// CUBE PRUNING
	// decoding
	void Decode(size_t stackInd);
	void PostDecode(size_t stackInd);
	void Prefetch(size_t stackInd);
};

}


