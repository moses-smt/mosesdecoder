/*
 * CubePruning.cpp
 *
 *  Created on: 27 Nov 2015
 *      Author: hieu
 */

#include "Misc.h"
#include "../Manager.h"
#include "../../MemPool.h"

using namespace std;

namespace Moses2
{

namespace NSCubePruning
{

////////////////////////////////////////////////////////////////////////
QueueItem::QueueItem(Manager &mgr, CubeEdge &edge, size_t hypoIndex, size_t tpIndex)
:edge(edge)
,hypoIndex(hypoIndex)
,tpIndex(tpIndex)
{
	CreateHypothesis(mgr);
}

void QueueItem::CreateHypothesis(Manager &mgr)
{
	const Hypothesis *prevHypo = edge.hypos[hypoIndex];
	const TargetPhrase &tp = edge.tps[tpIndex];

	//cerr << "hypoIndex=" << hypoIndex << endl;
	//cerr << "edge.hypos=" << edge.hypos.size() << endl;
	//cerr << prevHypo << endl;
	//cerr << *prevHypo << endl;

	hypo = Hypothesis::Create(mgr);
	hypo->Init(*prevHypo, edge.path, tp, edge.newBitmap, edge.estimatedScore);
	hypo->EvaluateWhenApplied();
}

////////////////////////////////////////////////////////////////////////
CubeEdge::CubeEdge(
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

std::ostream& operator<<(std::ostream &out, const CubeEdge &obj)
{
	out << obj.newBitmap;
	return out;
}

bool
CubeEdge::SeenPosition(const size_t x, const size_t y, SeenPositions &seenPositions) const
{
  std::pair<const CubeEdge*, int> val(this, (x<<16) + y);
  boost::unordered_set< std::pair<const CubeEdge*, int> >::iterator iter = seenPositions.find(val);
  return (iter != seenPositions.end());
}

void
CubeEdge::SetSeenPosition(const size_t x, const size_t y, SeenPositions &seenPositions) const
{
  //UTIL_THROW_IF2(x >= (1<<17), "Error");
  //UTIL_THROW_IF2(y >= (1<<17), "Error");

  std::pair<const CubeEdge*, int> val(this, (x<<16) + y);
  seenPositions.insert(val);
}

void CubeEdge::CreateFirst(Manager &mgr, Queue &queue, SeenPositions &seenPositions)
{
	assert(hypos.size());
	assert(tps.GetSize());

    MemPool &pool = mgr.GetPool();

	QueueItem *item = new (pool.Allocate<QueueItem>()) QueueItem(mgr, *this, 0, 0);
	queue.push(item);
	SetSeenPosition(0, 0, seenPositions);
}

void CubeEdge::CreateNext(Manager &mgr, const QueueItem *item, Queue &queue, SeenPositions &seenPositions)
{
    MemPool &pool = mgr.GetPool();

    size_t hypoIndex = item->hypoIndex + 1;
	if (hypoIndex < hypos.size() && !SeenPosition(hypoIndex, item->tpIndex, seenPositions)) {
		QueueItem *newItem = new (pool.Allocate<QueueItem>()) QueueItem(mgr, *this, hypoIndex, item->tpIndex);
		queue.push(newItem);

		SetSeenPosition(hypoIndex, item->tpIndex, seenPositions);
	}

	size_t tpIndex = item->tpIndex + 1;
	if (tpIndex < tps.GetSize() && !SeenPosition(item->hypoIndex, tpIndex, seenPositions)) {
		QueueItem *newItem = new (pool.Allocate<QueueItem>()) QueueItem(mgr, *this, item->hypoIndex, tpIndex);
		queue.push(newItem);

		SetSeenPosition(item->hypoIndex, tpIndex, seenPositions);
	}
}

void CubeEdge::Prefetch(Manager &mgr, const QueueItem *item, Queue &queue, SeenPositions &seenPositions)
{
    size_t hypoIndex = item->hypoIndex + 1;
	if (hypoIndex < hypos.size() && !SeenPosition(hypoIndex, item->tpIndex, seenPositions)) {
		const Hypothesis *hypo = hypos[hypoIndex];
		 __builtin_prefetch(hypo);

		const TargetPhrase &hypoTP = hypo->GetTargetPhrase();
		hypoTP.Prefetch();

		const TargetPhrase &tp = tps[item->tpIndex];
		tp.Prefetch();

	}

	size_t tpIndex = item->tpIndex + 1;
	if (tpIndex < tps.GetSize() && !SeenPosition(item->hypoIndex, tpIndex, seenPositions)) {
		const Hypothesis *hypo = hypos[item->hypoIndex];
		 __builtin_prefetch(hypo);

		const TargetPhrase &hypoTP = hypo->GetTargetPhrase();
		hypoTP.Prefetch();

		const TargetPhrase &tp = tps[tpIndex];
		tp.Prefetch();


	}
}

}

}


