/*
 * Search.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "Search.h"
#include "Stack.h"
#include "../Manager.h"
#include "../Hypothesis.h"
#include "../../InputPaths.h"
#include "../../InputPath.h"
#include "../../System.h"
#include "../../Sentence.h"
#include "../../TranslationTask.h"
#include "../../legacy/Util2.h"

using namespace std;

namespace Moses2
{

namespace NSCubePruningMiniStack
{

////////////////////////////////////////////////////////////////////////
Search::Search(Manager &mgr)
:Moses2::Search(mgr)
,m_stack(mgr)

,m_queue(QueueItemOrderer(), std::vector<QueueItem*>() )

,m_seenPositionsAlloc(mgr.GetPool())
,m_seenPositions(m_seenPositionsAlloc)
{
}

Search::~Search()
{
	for (size_t i = 0; i < m_cubeEdges.size(); ++i) {
		CubeEdges *edges = m_cubeEdges[i];
		delete edges;
	}
}

void Search::Decode()
{
	// init cue edges
	m_cubeEdges.resize(mgr.GetInput().GetSize() + 1);
	for (size_t i = 0; i < m_cubeEdges.size(); ++i) {
		m_cubeEdges[i] = new CubeEdges();
	}

	const Bitmap &initBitmap = mgr.GetBitmaps().GetInitialBitmap();
	Hypothesis *initHypo = Hypothesis::Create(mgr.GetSystemPool(), mgr);
	initHypo->Init(mgr, mgr.GetInputPaths().GetBlank(), mgr.GetInitPhrase(), initBitmap);
	initHypo->EmptyHypothesisState(mgr.GetInput());

	m_stack.Add(initHypo, mgr.GetHypoRecycle());
	PostDecode(0);

	for (size_t stackInd = 1; stackInd < mgr.GetInput().GetSize() + 1; ++stackInd) {
		//cerr << "stackInd=" << stackInd << endl;
		m_stack.Clear();
		Decode(stackInd);
		PostDecode(stackInd);

		//m_stack.DebugCounts();
		//cerr << m_stacks << endl;
	}

}

void Search::Decode(size_t stackInd)
{
	Recycler<Hypothesis*> &hypoRecycler  = mgr.GetHypoRecycle();

	// reuse queue from previous stack. Clear it first
	std::vector<QueueItem*> &container = Container(m_queue);
	//cerr << "container=" << container.size() << endl;
	BOOST_FOREACH(QueueItem *item, container) {
		// recycle unused hypos from queue
		Hypothesis *hypo = item->hypo;
		hypoRecycler.Recycle(hypo);

		// recycle queue item
		m_queueItemRecycler.push_back(item);
	}
	container.clear();

	m_seenPositions.clear();

	//Prefetch(stackInd);

	// add top hypo from every edge into queue
	CubeEdges &edges = *m_cubeEdges[stackInd];

	BOOST_FOREACH(CubeEdge *edge, edges) {
		//cerr << *edge << " ";
		edge->CreateFirst(mgr, m_queue, m_seenPositions, m_queueItemRecycler);
	}

	/*
	cerr << "edges: ";
	boost::unordered_set<const Bitmap*> uniqueBM;
	BOOST_FOREACH(CubeEdge *edge, edges) {
		uniqueBM.insert(&edge->newBitmap);
		//cerr << *edge << " ";
	}
	cerr << edges.size() << " " << uniqueBM.size();
	cerr << endl;
	*/

	size_t pops = 0;
	while (!m_queue.empty() && pops < mgr.system.popLimit) {
		// get best hypo from queue, add to stack
		//cerr << "queue=" << queue.size() << endl;
		QueueItem *item = m_queue.top();
		m_queue.pop();

		CubeEdge *edge = item->edge;

		// prefetching
		/*
		Hypothesis::Prefetch(mgr); // next hypo in recycler
		edge.Prefetch(mgr, item, m_queue, m_seenPositions); //next hypos of current item

		QueueItem *itemNext = m_queue.top();
		CubeEdge &edgeNext = itemNext->edge;
		edgeNext.Prefetch(mgr, itemNext, m_queue, m_seenPositions); //next hypos of NEXT item
		*/

		// add hypo to stack
		Hypothesis *hypo = item->hypo;

		if (mgr.system.cubePruningLazyScoring) {
			hypo->EvaluateWhenApplied();
		}

		//cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
		m_stack.Add(hypo, hypoRecycler);

		edge->CreateNext(mgr, item, m_queue, m_seenPositions, m_queueItemRecycler);

		++pops;
	}

	// create hypo from every edge. Increase diversity
	if (mgr.system.cubePruningDiversity) {
		while (!m_queue.empty()) {
			QueueItem *item = m_queue.top();
			m_queue.pop();

			if (item->hypoIndex == 0 && item->tpIndex == 0) {
				// add hypo to stack
				Hypothesis *hypo = item->hypo;
				//cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
				m_stack.Add(hypo, mgr.GetHypoRecycle());
			}
		}
	}
}

void Search::PostDecode(size_t stackInd)
{
  MemPool &pool = mgr.GetPool();

  const InputPaths &paths = mgr.GetInputPaths();
  const Matrix<InputPath*> &pathMatrix = paths.GetMatrix();
  size_t inputSize = pathMatrix.GetRows();
  size_t numPaths = pathMatrix.GetCols();

  BOOST_FOREACH(const Stack::Coll::value_type &val, m_stack.GetColl()) {
	  const Bitmap &hypoBitmap = *val.first.first;
	  size_t firstGap = hypoBitmap.GetFirstGapPos();
	  size_t hypoEndPos = val.first.second;
	  //cerr << "key=" << hypoBitmap << " " << firstGap << " " << inputSize << endl;

	  // create edges to next hypos from existing hypos
	  for (size_t startPos = firstGap; startPos < inputSize; ++startPos) {
		  for (size_t pathInd = 0; pathInd < numPaths; ++pathInd) {
			  const InputPath *path = pathMatrix.GetValue(startPos, pathInd);

		  		if (path == NULL) {
		  			break;
		  		}
		  		if (!path->IsUsed()) {
		  			continue;
		  		}

		  		const Range &pathRange = path->range;
		  		//cerr << "pathRange=" << pathRange << endl;
		  		if (!CanExtend(hypoBitmap, hypoEndPos, pathRange)) {
		  			continue;
		  		}

		  		const Bitmap &newBitmap = mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
		  		size_t numWords = newBitmap.GetNumWordsCovered();

		  		CubeEdges &edges = *m_cubeEdges[numWords];

				// sort hypo for a particular bitmap and hypoEndPos
		  		Hypotheses &sortedHypos = val.second->GetSortedAndPruneHypos(mgr);

		  	    size_t numPt = mgr.system.mappings.size();
		  	    for (size_t i = 0; i < numPt; ++i) {
		  	    	const TargetPhrases *tps = path->targetPhrases[i];
		  			if (tps && tps->GetSize()) {
		  		  		CubeEdge *edge = new (pool.Allocate<CubeEdge>()) CubeEdge(mgr, sortedHypos, *path, *tps, newBitmap);
		  		  		edges.push_back(edge);
		  			}
		  		}
		  }
	  }
  }
}

const Hypothesis *Search::GetBestHypothesis() const
{
	std::vector<const Hypothesis*> sortedHypos = m_stack.GetBestHypos(1);

	const Hypothesis *best = NULL;
	if (sortedHypos.size()) {
		best = sortedHypos[0];
	}
	return best;
}

void Search::Prefetch(size_t stackInd)
{
	CubeEdges &edges = *m_cubeEdges[stackInd];

	BOOST_FOREACH(CubeEdge *edge, edges) {
		 __builtin_prefetch(edge);

		 BOOST_FOREACH(const Hypothesis *hypo, edge->hypos) {
			 __builtin_prefetch(hypo);

			 const TargetPhrase &tp = hypo->GetTargetPhrase();
			 __builtin_prefetch(&tp);

		 }

		 BOOST_FOREACH(const TargetPhrase *tp, edge->tps) {
			 __builtin_prefetch(tp);

			 size_t size = tp->GetSize();
			 for (size_t i = 0; i < size; ++i) {
				 const Word &word = (*tp)[i];
				 __builtin_prefetch(&word);
			 }
		 }

	}
}

}

}


