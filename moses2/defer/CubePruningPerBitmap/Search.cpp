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

namespace NSCubePruningPerBitmap
{

////////////////////////////////////////////////////////////////////////
Search::Search(Manager &mgr)
  :Moses2::Search(mgr)
  ,m_stacks(mgr)

  ,m_queue(QueueItemOrderer(),
           std::vector<QueueItem*>() )

  ,m_seenPositions()
{
}

Search::~Search()
{
}

void Search::Decode()
{
  // init stacks
  m_stacks.Init(mgr.GetInput().GetSize() + 1);

  const Bitmap &initBitmap = mgr.GetBitmaps().GetInitialBitmap();
  Hypothesis *initHypo = Hypothesis::Create(mgr.GetSystemPool(), mgr);
  initHypo->Init(mgr, mgr.GetInputPaths().GetBlank(), mgr.GetInitPhrase(), initBitmap);
  initHypo->EmptyHypothesisState(mgr.GetInput());

  m_stacks.Add(initHypo, mgr.GetHypoRecycle());

  for (size_t stackInd = 0; stackInd < m_stacks.GetSize() - 1; ++stackInd) {
    CreateSearchGraph(stackInd);
  }

  for (size_t stackInd = 1; stackInd < m_stacks.GetSize(); ++stackInd) {
    //cerr << "stackInd=" << stackInd << endl;
    Decode(stackInd);

    //cerr << m_stacks << endl;
  }

  //DebugCounts();
}

void Search::Decode(size_t stackInd)
{
  NSCubePruningMiniStack::Stack &stack = m_stacks[stackInd];

  // FOR EACH BITMAP IN EACH STACK
  boost::unordered_map<const Bitmap*, vector<NSCubePruningMiniStack::MiniStack*> > uniqueBM;

  BOOST_FOREACH(NSCubePruningMiniStack::Stack::Coll::value_type &val, stack.GetColl()) {
    NSCubePruningMiniStack::MiniStack &miniStack = *val.second;

    const Bitmap *bitmap = val.first.first;
    uniqueBM[bitmap].push_back(&miniStack);
  }

  // decode each bitmap
  boost::unordered_map<const Bitmap*, vector<NSCubePruningMiniStack::MiniStack*> >::iterator iter;
  for (iter = uniqueBM.begin(); iter != uniqueBM.end(); ++iter) {
    const vector<NSCubePruningMiniStack::MiniStack*> &miniStacks = iter->second;
    Decode(miniStacks);
  }

  /*
  // FOR EACH STACK
  vector<NSCubePruningMiniStack::MiniStack*> miniStacks;
  BOOST_FOREACH(NSCubePruningMiniStack::Stack::Coll::value_type &val, stack.GetColl()) {
    NSCubePruningMiniStack::MiniStack &miniStack = *val.second;

    miniStacks.push_back(&miniStack);
  }
  Decode(miniStacks);
  */
}

void Search::Decode(const vector<NSCubePruningMiniStack::MiniStack*> &miniStacks)
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

  BOOST_FOREACH(NSCubePruningMiniStack::MiniStack *miniStack, miniStacks) {
    // add top hypo from every edge into queue
    CubeEdges &edges = *m_cubeEdges[miniStack];

    BOOST_FOREACH(CubeEdge *edge, edges) {
      //cerr << "edge=" << *edge << endl;
      edge->CreateFirst(mgr, m_queue, m_seenPositions, m_queueItemRecycler);
    }
  }

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
    m_stacks.Add(hypo, hypoRecycler);

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


void Search::CreateSearchGraph(size_t stackInd)
{
  NSCubePruningMiniStack::Stack &stack = m_stacks[stackInd];
  MemPool &pool = mgr.GetPool();

  BOOST_FOREACH(const NSCubePruningMiniStack::Stack::Coll::value_type &val, stack.GetColl()) {
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

      // sort hypo for a particular bitmap and hypoEndPos
      const NSCubePruningMiniStack::MiniStack &miniStack = *val.second;


      // add cube edge
      size_t numPt = mgr.system.mappings.size();
      for (size_t i = 0; i < numPt; ++i) {
        const TargetPhrases *tps = path->targetPhrases[i];
        if (tps && tps->GetSize()) {
          // create next mini stack
          NSCubePruningMiniStack::MiniStack &nextMiniStack = m_stacks.GetMiniStack(newBitmap, pathRange);

          CubeEdge *edge = new (pool.Allocate<CubeEdge>()) CubeEdge(mgr, miniStack, *path, *tps, newBitmap);

          CubeEdges *edges;
          boost::unordered_map<NSCubePruningMiniStack::MiniStack*, CubeEdges*>::iterator iter = m_cubeEdges.find(&nextMiniStack);
          if (iter == m_cubeEdges.end()) {
            edges = new (pool.Allocate<CubeEdges>()) CubeEdges();
            m_cubeEdges[&nextMiniStack] = edges;
          } else {
            edges = iter->second;
          }

          edges->push_back(edge);
        }
      }
    }
  }

}


const Hypothesis *Search::GetBestHypo() const
{
  const NSCubePruningMiniStack::Stack &lastStack = m_stacks.Back();
  std::vector<const Hypothesis*> sortedHypos = lastStack.GetBestHypos(1);

  const Hypothesis *best = NULL;
  if (sortedHypos.size()) {
    best = sortedHypos[0];
  }
  return best;
}

void Search::DebugCounts()
{
  std::map<size_t, size_t> counts;

  for (size_t stackInd = 0; stackInd < m_stacks.GetSize(); ++stackInd) {
    //cerr << "stackInd=" << stackInd << endl;
    const NSCubePruningMiniStack::Stack &stack = m_stacks[stackInd];
    BOOST_FOREACH(const NSCubePruningMiniStack::Stack::Coll::value_type &val, stack.GetColl()) {
      const NSCubePruningMiniStack::MiniStack &miniStack = *val.second;
      size_t count = miniStack.GetColl().size();

      if (counts.find(count) == counts.end()) {
        counts[count] = 0;
      } else {
        ++counts[count];
      }
    }
    //cerr << m_stacks << endl;
  }

  std::map<size_t, size_t>::const_iterator iter;
  for (iter = counts.begin(); iter != counts.end(); ++iter) {
    cerr << iter->first << "=" << iter->second << " ";
  }
  cerr << endl;
}



}

}


