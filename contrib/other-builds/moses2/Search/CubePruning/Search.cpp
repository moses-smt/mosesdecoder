/*
 * Search.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "Search.h"
#include "../Manager.h"
#include "../Hypothesis.h"
#include "../../InputPaths.h"
#include "../../InputPath.h"
#include "../../System.h"
#include "../../TranslationTask.h"
#include "../../legacy/Util2.h"

using namespace std;

namespace NSCubePruning
{

////////////////////////////////////////////////////////////////////////
Search::Search(Manager &mgr)
: ::Search(mgr)
{
}

Search::~Search() {
}

void Search::Decode()
{
	// init stacks
	m_stacks.Init(m_mgr.GetInput().GetSize() + 1);
	m_cubeEdges.resize(m_stacks.GetSize() + 1);

	const Bitmap &initBitmap = m_mgr.GetBitmaps().GetInitialBitmap();
	Hypothesis *initHypo = Hypothesis::Create(m_mgr);
	initHypo->Init(m_mgr.GetInitPhrase(), m_mgr.GetInitRange(), initBitmap);
	initHypo->EmptyHypothesisState(m_mgr.GetInput());

	m_stacks.Add(initHypo, m_mgr.GetHypoRecycle());

	for (size_t stackInd = 0; stackInd < m_stacks.GetSize(); ++stackInd) {
		//cerr << "stackInd=" << stackInd << endl;
		Decode(stackInd);
		PostDecode(stackInd);

		//cerr << m_stacks << endl;

		// delete stack to save mem
		if (stackInd < m_stacks.GetSize() - 1) {
			//m_stacks.Delete(stackInd);
		}
		//cerr << m_stacks << endl;
	}

}

// grab the underlying contain of priority queue
/////////////////////////////////////////////////
template <class T, class S, class C>
    S& Container(priority_queue<T, S, C>& q) {
        struct HackedQueue : private priority_queue<T, S, C> {
            static S& Container(priority_queue<T, S, C>& q) {
                return q.*&HackedQueue::c;
            }
        };
    return HackedQueue::Container(q);
}
/////////////////////////////////////////////////

void Search::Decode(size_t stackInd)
{
	NSCubePruning::CubeEdge::Queue &queue = m_mgr.task.queue;
	std::vector<QueueItem*> &queueContainer = Container(queue);
	queueContainer.clear();

	NSCubePruning::CubeEdge::SeenPositions &seenPositions = m_mgr.task.seenPositions;
	seenPositions.clear();

	//Prefetch(stackInd);

	// add top hypo from every edge into queue
	CubeEdges &edges = m_cubeEdges[stackInd];

	BOOST_FOREACH(CubeEdge *edge, edges) {
		//cerr << "edge=" << *edge << endl;
		edge->CreateFirst(m_mgr, queue, seenPositions);
	}

	/*
	cerr << "queue:" << endl;
	vector<QueueItem*> &queueContainer = Container(queue);
	for (size_t i = 0; i < queueContainer.size(); ++i) {
		QueueItem *item = queueContainer[i];
		Hypothesis *hypo = item->hypo;
		cerr << *hypo << endl;
	}
	cerr << endl;
	*/

	size_t pops = 0;
	while (!queue.empty() && pops < m_mgr.system.popLimit) {
		// get best hypo from queue, add to stack
		//cerr << "queue=" << queue.size() << endl;
		QueueItem *item = queue.top();
		queue.pop();

		Hypothesis::Prefetch(m_mgr);

		CubeEdge &edge = item->edge;
		edge.Prefetch(m_mgr, item, queue, seenPositions);

		Hypothesis *hypo = item->hypo;
		//cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
		m_stacks.Add(hypo, m_mgr.GetHypoRecycle());

		edge.CreateNext(m_mgr, item, queue, seenPositions);

		++pops;
	}
}

void Search::PostDecode(size_t stackInd)
{
  Stack &stack = m_stacks[stackInd];
  MemPool &pool = m_mgr.GetPool();

  BOOST_FOREACH(const Stack::Coll::value_type &val, stack.GetColl()) {
	  const Bitmap &hypoBitmap = *val.first.first;
	  size_t hypoEndPos = val.first.second;
	  //cerr << "key=" << hypoBitmap << " " << hypoEndPos << endl;

	  // create edges to next hypos from existing hypos
	  const InputPaths &paths = m_mgr.GetInputPaths();

	  BOOST_FOREACH(const InputPath &path, paths) {
  		const Range &pathRange = path.range;
  		//cerr << "pathRange=" << pathRange << endl;

  		if (!path.IsUsed()) {
  			continue;
  		}
  		if (!CanExtend(hypoBitmap, hypoEndPos, pathRange)) {
  			continue;
  		}

  		const Bitmap &newBitmap = m_mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
  		size_t numWords = newBitmap.GetNumWordsCovered();

  		CubeEdges &edges = m_cubeEdges[numWords];

		// sort hypo for a particular bitmap and hypoEndPos
		CubeEdge::Hypotheses &sortedHypos = val.second.GetSortedHypos(m_mgr);

  		BOOST_FOREACH(const TargetPhrases::shared_const_ptr &tpsPtr, path.targetPhrases) {
  			const TargetPhrases *tps = tpsPtr.get();
  			if (tps && tps->GetSize()) {
  		  		CubeEdge *edge = new (pool.Allocate<CubeEdge>()) CubeEdge(m_mgr, sortedHypos, path, *tps, newBitmap);
  		  		edges.push_back(edge);
  			}
  		}
  	  }
  }
}

const Hypothesis *Search::GetBestHypothesis() const
{
	const Stack &lastStack = m_stacks.Back();
	std::vector<const Hypothesis*> sortedHypos = lastStack.GetBestHypos(1);

	const Hypothesis *best = NULL;
	if (sortedHypos.size()) {
		best = sortedHypos[0];
	}
	return best;
}

void Search::Prefetch(size_t stackInd)
{
	CubeEdges &edges = m_cubeEdges[stackInd];

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


