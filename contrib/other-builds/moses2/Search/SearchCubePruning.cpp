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

////////////////////////////////////////////////////////////////////////
SearchCubePruning::SearchCubePruning(Manager &mgr)
:Search(mgr)
{
}

SearchCubePruning::~SearchCubePruning() {
	// TODO Auto-generated destructor stub
}

void SearchCubePruning::Decode()
{
	// init stacks
	m_stacks.Init(m_mgr.GetInput().GetSize() + 1);
	m_hyposForCube.resize(m_stacks.GetSize() + 1);
	m_cubeEdges.resize(m_stacks.GetSize() + 1);

	const Bitmap &initBitmap = m_mgr.GetBitmaps().GetInitialBitmap();
	Hypothesis *initHypo = Hypothesis::Create(m_mgr);
	initHypo->Init(m_mgr.GetInitPhrase(), m_mgr.GetInitRange(), initBitmap);
	initHypo->EmptyHypothesisState(m_mgr.GetInput());

	m_stacks.Add(initHypo, m_mgr.GetHypoRecycle());

	PostDecode(0);

	for (size_t stackInd = 0; stackInd < m_stacks.GetSize(); ++stackInd) {
		Decode(stackInd);
		PostDecode(stackInd);

		cerr << m_stacks << endl;

		// delete stack to save mem
		if (stackInd < m_stacks.GetSize() - 1) {
			m_stacks.Delete(stackInd);
		}
		//cerr << m_stacks << endl;
	}

}

void SearchCubePruning::Decode(size_t stackInd)
{

	CubeEdge::Queue queue;

	// add top hypo from every edge into queue
	std::vector<CubeEdge*> &edges = m_cubeEdges[stackInd];
	BOOST_FOREACH(CubeEdge *edge, edges) {
		//cerr << "edge=" << *edge << endl;
		edge->CreateFirst(m_mgr, queue);
	}

	size_t pops = 0;
	while (!queue.empty() && pops < m_mgr.system.popLimit) {
		// get best hypo from queue, add to stack
		//cerr << "queue=" << queue.size() << endl;
		QueueItem *ele = queue.top();
		queue.pop();

		Hypothesis *hypo = ele->hypo;
		//cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
		m_stacks.Add(hypo, m_mgr.GetHypoRecycle());

		CubeEdge &edge = ele->edge;
		edge.CreateNext(m_mgr, *ele, queue);

		delete ele;
		++pops;
	}

	RemoveAllInColl(edges);
}

void SearchCubePruning::PostDecode(size_t stackInd)
{
  StackCubePruning &stack = m_stacks[stackInd];
  HyposForCubePruning &hyposPerBMAndRange = m_hyposForCube[stackInd];

  BOOST_FOREACH(const StackCubePruning::Coll &val, stack.m_coll) {

  }

  // create list of hypos in this stack, sorted by bitmap and range
  BOOST_FOREACH(const Hypothesis *hypo, stack) {
	  const Bitmap &hypoBitmap = hypo->GetBitmap();
	  const Range &hypoRange = hypo->GetRange();

	  CubeEdge::Hypotheses &hypos = hyposPerBMAndRange.GetOrCreate(hypoBitmap, hypoRange);
	  hypos.push_back(hypo);
  }

  // sort and prune hypos
  SortAndPruneHypos(hyposPerBMAndRange);

  // create edges to next hypos from existing hypos
  const InputPaths &paths = m_mgr.GetInputPaths();

  BOOST_FOREACH(HyposForCubePruning::value_type &val, hyposPerBMAndRange) {
	  const HyposForCubePruning::HypoCoverage &hypoCoverage = val.first;
	  const Bitmap &hypoBitmap = *hypoCoverage.first;
	  const Range &hypoRange = *hypoCoverage.second;

	  const CubeEdge::Hypotheses &hypos = val.second;
	  //cerr << "hypos=" << hypos.size() << endl;

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
  		  		CubeEdge *edge = new CubeEdge(m_mgr, hypos, path, *tps, newBitmap);
  		  		std::vector<CubeEdge*> &edges = m_cubeEdges[numWords];
  		  		edges.push_back(edge);
  			}
  		}
  	  }
  }
}

void SearchCubePruning::SortAndPruneHypos(HyposForCubePruning &hyposPerBMAndRange)
{
  size_t stackSize = m_mgr.system.stackSize;
  Recycler<Hypothesis*> &recycler = m_mgr.GetHypoRecycle();

  BOOST_FOREACH(HyposForCubePruning::value_type &val, hyposPerBMAndRange) {
	  CubeEdge::Hypotheses &hypos = val.second;

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

const Hypothesis *SearchCubePruning::GetBestHypothesis() const
{
	const StackCubePruning &lastStack = m_stacks.Back();
	std::vector<const Hypothesis*> sortedHypos = lastStack.GetBestHypos(1);

	const Hypothesis *best = NULL;
	if (sortedHypos.size()) {
		best = sortedHypos[0];
	}
	return best;
}
