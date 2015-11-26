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
		{}

		const Hypotheses &hypos;
		const InputPath &path;
		const TargetPhrases &tps;
		const Bitmap &newBitmap;
	};

	std::vector<std::vector<CubeEdge> > m_cubeEdges;

	// CUBE PRUNING
	// decoding
	struct CubeElement
	{
		CubeElement(Manager &mgr, const CubeEdge &edge, size_t hypoIndex, size_t tpIndex)
		:edge(edge)
		,hypoIndex(hypoIndex)
		,tpIndex(tpIndex)
		{
			CreateHypothesis(mgr);
		}

		const CubeEdge &edge;
		size_t hypoIndex, tpIndex;
		Hypothesis *hypo;

		void CreateHypothesis(Manager &mgr);
};

	std::queue<CubeElement*> m_queue;

	void SortAndPruneHypos();
};

