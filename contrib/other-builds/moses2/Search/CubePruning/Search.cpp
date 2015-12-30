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
#include "../../Sentence.h"
#include "../../TranslationTask.h"
#include "../../legacy/Util2.h"

using namespace std;

namespace Moses2
{

namespace NSCubePruning
{

////////////////////////////////////////////////////////////////////////
Search::Search(Manager &mgr)
:Moses2::Search(mgr)
,m_stacks(mgr)

,m_queueOrder(new QueueItemOrderer())
,m_queueContainerAlloc(new boost::fast_pool_allocator<QueueItem*>())
,m_queueContainer(new std::vector<QueueItem*, boost::fast_pool_allocator<QueueItem*> >(*m_queueContainerAlloc))
,m_queue(new CubeEdge::Queue(*m_queueOrder, *m_queueContainer))

,m_seenPositionsAlloc(new boost::pool_allocator<CubeEdge::SeenPositionItem>())
,m_seenPositions(new CubeEdge::SeenPositions(*m_seenPositionsAlloc))
{
}

Search::~Search()
{
	BOOST_FOREACH(CubeEdges &edges, m_cubeEdges) {
		RemoveAllInColl(edges);
	}

	delete m_queue;
	delete m_queueContainer;
	delete m_queueContainerAlloc;
	delete m_queueOrder;

	delete m_seenPositions;
	delete m_seenPositionsAlloc;
}

void Search::Decode()
{
	// init stacks
	m_stacks.Init(m_mgr.GetInput().GetSize() + 1);
	m_cubeEdges.resize(m_stacks.GetSize() + 1);

	const Bitmap &initBitmap = m_mgr.GetBitmaps().GetInitialBitmap();
	Hypothesis *initHypo = Hypothesis::Create(m_mgr);
	initHypo->Init(m_mgr.GetInputPaths().GetBlank(), m_mgr.GetInitPhrase(), initBitmap);
	initHypo->EmptyHypothesisState(m_mgr.GetInput());

	m_stacks.Add(initHypo, m_mgr.GetHypoRecycle());

	for (size_t stackInd = 0; stackInd < m_stacks.GetSize(); ++stackInd) {
		//cerr << "stackInd=" << stackInd << endl;
		Decode(stackInd);
		PostDecode(stackInd);

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
	std::vector<QueueItem*, boost::fast_pool_allocator<QueueItem*> > &container = Container(*m_queue);
	container.clear();
	//m_queueContainer->clear();
	m_seenPositions->clear();

	//Prefetch(stackInd);

	// add top hypo from every edge into queue
	CubeEdges &edges = m_cubeEdges[stackInd];

	BOOST_FOREACH(CubeEdge *edge, edges) {
		//cerr << "edge=" << *edge << endl;
		edge->CreateFirst(m_mgr, *m_queue, *m_seenPositions);
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
	while (!m_queue->empty() && pops < m_mgr.system.popLimit) {
		// get best hypo from queue, add to stack
		//cerr << "queue=" << queue.size() << endl;
		QueueItem *item = m_queue->top();
		m_queue->pop();

		CubeEdge &edge = item->edge;

		// prefetching
		/*
		Hypothesis::Prefetch(m_mgr); // next hypo in recycler
		edge.Prefetch(m_mgr, item, m_queue, m_seenPositions); //next hypos of current item

		QueueItem *itemNext = m_queue.top();
		CubeEdge &edgeNext = itemNext->edge;
		edgeNext.Prefetch(m_mgr, itemNext, m_queue, m_seenPositions); //next hypos of NEXT item
		*/

		// add hypo to stack
		Hypothesis *hypo = item->hypo;
		//cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
		m_stacks.Add(hypo, m_mgr.GetHypoRecycle());

		edge.CreateNext(m_mgr, item, *m_queue, *m_seenPositions);

		++pops;
	}

	/*
	// create hypo from every edge. Increase diversity
	while (!m_queue.empty()) {
		QueueItem *item = m_queue.top();
		m_queue.pop();

		if (item->hypoIndex == 0 && item->tpIndex == 0) {
			CubeEdge &edge = item->edge;

			// add hypo to stack
			Hypothesis *hypo = item->hypo;
			//cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
			m_stacks.Add(hypo, m_mgr.GetHypoRecycle());
		}
	}
	*/
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
		CubeEdge::Hypotheses &sortedHypos = val.second.GetSortedAndPruneHypos(m_mgr);

  		BOOST_FOREACH(const TargetPhrases *tps, path.targetPhrases) {
  			if (tps && tps->GetSize()) {
  		  		CubeEdge *edge = new CubeEdge(m_mgr, sortedHypos, path, *tps, newBitmap);
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

}


