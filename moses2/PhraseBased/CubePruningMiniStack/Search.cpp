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
#include "../TrellisPath.h"
#include "../Sentence.h"
#include "../../TrellisPaths.h"
#include "../../InputPathsBase.h"
#include "../../InputPathBase.h"
#include "../../System.h"
#include "../../TranslationTask.h"
#include "../../legacy/Util2.h"
#include "../../PhraseBased/TargetPhrases.h"

using namespace std;

namespace Moses2
{

namespace NSCubePruningMiniStack
{

////////////////////////////////////////////////////////////////////////
Search::Search(Manager &mgr) :
  Moses2::Search(mgr), m_stack(mgr), m_cubeEdgeAlloc(mgr.GetPool())

  , m_queue(QueueItemOrderer(),
            std::vector<QueueItem*, MemPoolAllocator<QueueItem*> >(
              MemPoolAllocator<QueueItem*>(mgr.GetPool())))

  , m_seenPositions(
    MemPoolAllocator<CubeEdge::SeenPositionItem>(mgr.GetPool()))

  , m_queueItemRecycler(MemPoolAllocator<QueueItem*>(mgr.GetPool()))

{
}

Search::~Search()
{
}

void Search::Decode()
{
  const Sentence &sentence = static_cast<const Sentence&>(mgr.GetInput());

  // init cue edges
  m_cubeEdges.resize(sentence.GetSize() + 1);
  for (size_t i = 0; i < m_cubeEdges.size(); ++i) {
    m_cubeEdges[i] = new (mgr.GetPool().Allocate<CubeEdges>()) CubeEdges(
      m_cubeEdgeAlloc);
  }

  const Bitmap &initBitmap = mgr.GetBitmaps().GetInitialBitmap();
  Hypothesis *initHypo = Hypothesis::Create(mgr.GetSystemPool(), mgr);
  initHypo->Init(mgr, mgr.GetInputPaths().GetBlank(), mgr.GetInitPhrase(),
                 initBitmap);
  initHypo->EmptyHypothesisState(mgr.GetInput());
  //cerr << "initHypo=" << *initHypo << endl;

  m_stack.Add(initHypo, mgr.GetHypoRecycle(), mgr.arcLists);
  PostDecode(0);

  for (size_t stackInd = 1; stackInd < sentence.GetSize() + 1;
       ++stackInd) {
    //cerr << "stackInd=" << stackInd << endl;
    m_stack.Clear();
    Decode(stackInd);
    PostDecode(stackInd);

    //m_stack.DebugCounts();
  }

}

void Search::Decode(size_t stackInd)
{
  Recycler<HypothesisBase*> &hypoRecycler = mgr.GetHypoRecycle();

  // reuse queue from previous stack. Clear it first
  std::vector<QueueItem*, MemPoolAllocator<QueueItem*> > &container = Container(
        m_queue);
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
  while (!m_queue.empty() && pops < mgr.system.options.cube.pop_limit) {
    // get best hypo from queue, add to stack
    //cerr << "queue=" << queue.size() << endl;
    QueueItem *item = m_queue.top();
    m_queue.pop();

    CubeEdge *edge = item->edge;

    // add hypo to stack
    Hypothesis *hypo = item->hypo;

    if (mgr.system.options.cube.lazy_scoring) {
      hypo->EvaluateWhenApplied();
    }

    //cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
    m_stack.Add(hypo, hypoRecycler, mgr.arcLists);

    edge->CreateNext(mgr, item, m_queue, m_seenPositions, m_queueItemRecycler);

    ++pops;
  }

  // create hypo from every edge. Increase diversity
  if (mgr.system.options.cube.diversity) {
    while (!m_queue.empty()) {
      QueueItem *item = m_queue.top();
      m_queue.pop();

      if (item->hypoIndex == 0 && item->tpIndex == 0) {
        // add hypo to stack
        Hypothesis *hypo = item->hypo;
        //cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
        m_stack.Add(hypo, hypoRecycler, mgr.arcLists);
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

    Moses2::HypothesisColl &hypos = *val.second;

    //cerr << "key=" << hypoBitmap << " " << firstGap << " " << inputSize << endl;

    // create edges to next hypos from existing hypos
    for (size_t startPos = firstGap; startPos < inputSize; ++startPos) {
      for (size_t pathInd = 0; pathInd < numPaths; ++pathInd) {
        const InputPath *path = pathMatrix.GetValue(startPos, pathInd);

        if (path == NULL) {
          break;
        }
        if (path->GetNumRules() == 0) {
          continue;
        }

        const Range &pathRange = path->range;
        //cerr << "pathRange=" << pathRange << endl;
        if (!CanExtend(hypoBitmap, hypoEndPos, pathRange)) {
          continue;
        }

        const ReorderingConstraint &reorderingConstraint = mgr.GetInput().GetReorderingConstraint();
        if (!reorderingConstraint.Check(hypoBitmap, startPos, pathRange.GetEndPos())) {
          continue;
        }

        const Bitmap &newBitmap = mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
        size_t numWords = newBitmap.GetNumWordsCovered();

        CubeEdges &edges = *m_cubeEdges[numWords];

        // sort hypo for a particular bitmap and hypoEndPos
        const Hypotheses &sortedHypos = hypos.GetSortedAndPrunedHypos(mgr, mgr.arcLists);

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

const Hypothesis *Search::GetBestHypo() const
{
  const Hypothesis *bestHypo = m_stack.GetBestHypo();
  return bestHypo;
}

void Search::AddInitialTrellisPaths(TrellisPaths<TrellisPath> &paths) const
{
  const Stack::Coll &coll = m_stack.GetColl();
  BOOST_FOREACH(const Stack::Coll::value_type &val, coll) {
    Moses2::HypothesisColl &hypos = *val.second;
    const Hypotheses &sortedHypos = hypos.GetSortedAndPrunedHypos(mgr, mgr.arcLists);

    BOOST_FOREACH(const HypothesisBase *hypoBase, sortedHypos) {
      const Hypothesis *hypo = static_cast<const Hypothesis*>(hypoBase);
      TrellisPath *path = new TrellisPath(hypo, mgr.arcLists);
      paths.Add(path);
    }
  }
}

}

}

