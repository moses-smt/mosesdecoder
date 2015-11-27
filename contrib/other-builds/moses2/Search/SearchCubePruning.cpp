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
#include "../System.h"

using namespace std;

SearchCubePruning::CubeEdge::CubeEdge(
		Manager &mgr,
		const Hypotheses &hypos,
		const InputPath &path,
		const TargetPhrases &tps,
		const Bitmap &newBitmap)
:hypos(hypos)
,path(path)
,tps(tps)
,newBitmap(newBitmap)
{
	estimatedScore = mgr.GetEstimatedScores().CalcEstimatedScore(newBitmap);
}

////////////////////////////////////////////////////////////////////////
void SearchCubePruning::CubeElement::CreateHypothesis(Manager &mgr)
{
	const Hypothesis &prevHypo = *edge.hypos[hypoIndex];
	const TargetPhrase &tp = edge.tps[tpIndex];

	hypo = Hypothesis::Create(mgr);
	hypo->Init(prevHypo, tp, edge.path.range, edge.newBitmap, edge.estimatedScore);
}

////////////////////////////////////////////////////////////////////////
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
	// add top hypo from every edge into queue
	std::vector<CubeEdge> &edges = m_cubeEdges[stackInd];
	BOOST_FOREACH(const CubeEdge &edge, edges) {
		CubeElement *ele = new CubeElement(m_mgr, edge, 0, 0);
		m_queue.push(ele);
	}

	cerr << "stack " << stackInd << " " << m_queue.size() << endl;
	if (!m_queue.empty()) {
		// get best hypo from queue, add to stack
		CubeElement *ele = m_queue.front();
		Hypothesis *hypo = ele->hypo;
		m_stacks.Add(hypo, m_mgr.GetHypoRecycle());
	}
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
  SortAndPruneHypos();

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

  		BOOST_FOREACH(const TargetPhrases::shared_const_ptr &tpsPtr, path.targetPhrases) {
  			const TargetPhrases *tps = tpsPtr.get();
  			if (tps && tps->GetSize()) {
  		  		CubeEdge edge(m_mgr, hypos, path, *tps, newBitmap);
  		  		std::vector<CubeEdge> &edges = m_cubeEdges[numWords];
  		  		edges.push_back(edge);
  			}
  		}
  	  }
  }
}

void SearchCubePruning::SortAndPruneHypos()
{
  size_t stackSize = m_mgr.system.stackSize;
  Recycler<Hypothesis*> &recycler = m_mgr.GetHypoRecycle();

  BOOST_FOREACH(HyposForCube::value_type val, m_hyposForCube) {
	  Hypotheses &hypos = val.second;

	  std::vector<const Hypothesis*>::iterator iterMiddle;
	  iterMiddle = (stackSize == 0 || hypos.size() < stackSize)
				   ? hypos.end()
				   : hypos.begin() + stackSize;

	  std::partial_sort(hypos.begin(), iterMiddle, hypos.end(),
			  HypothesisFutureScoreOrderer());

	  // prune
	  if (stackSize && hypos.size() > stackSize) {
		  for (size_t i = stackSize; i < hypos.size(); ++i) {
			  Hypothesis *hypo = const_cast<Hypothesis*>(hypos[i]);
			  recycler.Add(hypo);
		  }
		  hypos.resize(stackSize);
	  }
  }
}

