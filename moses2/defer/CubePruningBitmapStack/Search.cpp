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

namespace NSCubePruningBitmapStack
{

////////////////////////////////////////////////////////////////////////
Search::Search(Manager &mgr)
  :Moses2::Search(mgr)
  ,m_stack(mgr)

  ,m_queue(QueueItemOrderer(), std::vector<QueueItem*>() )

  ,m_seenPositions()
{
}

Search::~Search()
{
}

void Search::Decode()
{
  // init cue edges
  m_cubeEdges.resize(mgr.GetInput().GetSize() + 1);
  for (size_t i = 0; i < m_cubeEdges.size(); ++i) {
    m_cubeEdges[i] = new (mgr.GetPool().Allocate<CubeEdges>()) CubeEdges();
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

    // add hypo to stack
    Hypothesis *hypo = item->hypo;
    //cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
    m_stack.Add(hypo, hypoRecycler);

    edge->CreateNext(mgr, item, m_queue, m_seenPositions, m_queueItemRecycler);

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
  		m_stacks.Add(hypo, mgr.GetHypoRecycle());
  	}
  }
  */
}

void Search::PostDecode(size_t stackInd)
{
  MemPool &pool = mgr.GetPool();

  Stack::SortedHypos sortedHypos = m_stack.GetSortedAndPruneHypos(mgr);

  BOOST_FOREACH(const Stack::SortedHypos::value_type &val, sortedHypos) {
    const Bitmap &hypoBitmap = *val.first.first;
    size_t hypoEndPos = val.first.second;
    //cerr << "key=" << hypoBitmap << " " << hypoEndPos << endl;

    // create edges to next hypos from existing hypos
    const InputPaths &paths = mgr.GetInputPaths();

    BOOST_FOREACH(const InputPath *path, paths) {
      const Range &pathRange = path->range;
      //cerr << "pathRange=" << pathRange << endl;

      if (!path->IsUsed()) {
        continue;
      }
      if (!CanExtend(hypoBitmap, hypoEndPos, pathRange)) {
        continue;
      }

      const Bitmap &newBitmap = mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
      size_t numWords = newBitmap.GetNumWordsCovered();

      CubeEdges &edges = *m_cubeEdges[numWords];

      // sort hypo for a particular bitmap and hypoEndPos
      Hypotheses &sortedHypos = *val.second;

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

const Hypothesis *Search::GetBestHypo() const
{
  std::vector<const Hypothesis*> sortedHypos = m_stack.GetBestHypos(1);

  const Hypothesis *best = NULL;
  if (sortedHypos.size()) {
    best = sortedHypos[0];
  }
  return best;
}

}

}


