/*
 * SearchCubePruning.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <boost/unordered_map.hpp>
#include <vector>
#include <queue>
#include "Search.h"
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
	typedef std::vector<const Hypothesis*>  Hypotheses;
	typedef std::pair<const Bitmap*, Range> HypoCoverage;
	  // bitmap and range of hypos
	typedef boost::unordered_map<HypoCoverage, Hypotheses> HyposForCube;
	HyposForCube m_hyposForCube;

	struct CubeEdge
	{
		CubeEdge(const Hypotheses &hypos,
				const InputPath &path,
				const TargetPhrases &tps,
				const Bitmap &newBitmap)
		:hypos(hypos)
		,path(path)
		,tps(tps)
		,newBitmap(newBitmap)
		,hypoIndex(0)
		,tpIndex(0)
		{}

		const Hypotheses &hypos;
		const InputPath &path;
		const TargetPhrases &tps;
		const Bitmap &newBitmap;

		size_t hypoIndex, tpIndex;
	};

	std::vector<std::vector<CubeEdge> > m_cubeEdges;

	// CUBE PRUNING
	// decoding
	std::queue<Hypotheses*> m_queue;

	void SortAndPruneHypos();
};

