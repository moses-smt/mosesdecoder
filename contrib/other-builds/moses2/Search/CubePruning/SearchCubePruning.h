/*
 * SearchCubePruning.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#pragma once

#include "../Search.h"
#include "Misc.h"
#include "StacksCubePruning.h"
#include "../../legacy/Range.h"

class Bitmap;
class Hypothesis;
class InputPath;
class TargetPhrases;

class SearchCubePruning : public Search
{
public:
	SearchCubePruning(Manager &mgr);
	virtual ~SearchCubePruning();

	virtual void Decode();
	const Hypothesis *GetBestHypothesis() const;

protected:
	StacksCubePruning m_stacks;

	// CUBE PRUNING VARIABLES
	// setup
	std::vector<HyposForCubePruning> m_hyposForCube;
	   // pruned set of hypos, separated by [Bitmap, Range], for ALL stacks

	std::vector<std::vector<CubeEdge*> > m_cubeEdges;

	// CUBE PRUNING
	// decoding
	void Decode(size_t stackInd);
	void PostDecode(size_t stackInd);

	void SortAndPruneHypos(CubeEdge::Hypotheses &hypos);

};

