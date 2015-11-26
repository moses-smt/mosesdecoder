/*
 * SearchCubePruning.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "SearchCubePruning.h"
#include "Stacks.h"
#include "Stack.h"
#include "Manager.h"
#include "Hypothesis.h"
#include "../InputPaths.h"
#include "../InputPath.h"

SearchCubePruning::SearchCubePruning(Manager &mgr, Stacks &stacks)
:Search(mgr, stacks)
{
	m_cubeEdges.resize(stacks.GetSize());
}

SearchCubePruning::~SearchCubePruning() {
	// TODO Auto-generated destructor stub
}

void SearchCubePruning::Decode(size_t stackInd)
{

}

void SearchCubePruning::PostDecode(size_t stackInd)
{
  Stack &stack = m_stacks[stackInd];

  // create list of hypos in this stack, sorted by bitmap and range
  BOOST_FOREACH(const Hypothesis *hypo, stack) {
	  const Bitmap &hypoBitmap = hypo->GetBitmap();
	  const Range &hypoRange = hypo->GetRange();

	  Hypotheses &hypos = m_hyposForCube[HypoCoverage(&hypoBitmap, hypoRange)];
	  hypos.push_back(hypo);
  }

  // sort and prune hypos
  // TODO

  // create edges to next hypos from existing hypos
  const InputPaths &paths = m_mgr.GetInputPaths();

  BOOST_FOREACH(HyposForCube::value_type val, m_hyposForCube) {
	  const HypoCoverage &hypoCoverage = val.first;
	  const Bitmap &hypoBitmap = *hypoCoverage.first;
	  const Range &hypoRange = hypoCoverage.second;

	  const Hypotheses &hypos = val.second;

  	  BOOST_FOREACH(const InputPath &path, paths) {
  		const Range &pathRange = path.range;

  		if (!CanExtend(hypoBitmap, hypoRange, pathRange)) {
  			continue;
  		}

  		const Bitmap &newBitmap = m_mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
  		size_t numWords = newBitmap.GetNumWordsCovered();

  		CubeEdge edge;
  		edge.hypos = &hypos;
  		edge.path = &path;
  		edge.newBitmap = &newBitmap;

  		std::vector<CubeEdge> &edges = m_cubeEdges[numWords];
  		edges.push_back(edge);
  	  }
  }
}

