/*
 * SearchCubePruning.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#pragma once

#include "Search.h"
#include "CubePruning.h"
#include "../legacy/Range.h"

class Bitmap;
class Hypothesis;
class InputPath;
class TargetPhrases;

class SearchCubePruning : public Search
{
public:
	SearchCubePruning(Manager &mgr, Stacks &stacks);
	virtual ~SearchCubePruning();

	void Decode(size_t stackInd);
	void PostDecode(size_t stackInd);

protected:

	// CUBE PRUNING VARIABLES
	// setup
	std::vector<CubeEdge::HyposForCube> m_hyposForCube;
	   // pruned set of hypos, separated by [Bitmap, Range], for ALL stacks

	std::vector<std::vector<CubeEdge*> > m_cubeEdges;

	// CUBE PRUNING
	// decoding

	void SortAndPruneHypos(CubeEdge::HyposForCube &hyposPerBMAndRange);
};

