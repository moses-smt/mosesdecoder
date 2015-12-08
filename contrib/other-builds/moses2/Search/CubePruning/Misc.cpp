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
	hypo->Init(*prevHypo, tp, edge.path.range, edge.newBitmap, edge.estimatedScore);
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

	QueueItem *newEle = new (pool.Allocate<QueueItem>()) QueueItem(mgr, *this, 0, 0);
	queue.push(newEle);
	SetSeenPosition(0, 0, seenPositions);
}

void CubeEdge::CreateNext(Manager &mgr, const QueueItem *ele, Queue &queue, SeenPositions &seenPositions)
{
    MemPool &pool = mgr.GetPool();

    size_t hypoIndex = ele->hypoIndex + 1;
	if (hypoIndex < hypos.size() && !SeenPosition(hypoIndex, ele->tpIndex, seenPositions)) {
		QueueItem *newEle = new (pool.Allocate<QueueItem>()) QueueItem(mgr, *this, hypoIndex, ele->tpIndex);
		queue.push(newEle);

		SetSeenPosition(hypoIndex, ele->tpIndex, seenPositions);
	}

	size_t tpIndex = ele->tpIndex + 1;
	if (tpIndex < tps.GetSize() && !SeenPosition(ele->hypoIndex, tpIndex, seenPositions)) {
		QueueItem *newEle = new (pool.Allocate<QueueItem>()) QueueItem(mgr, *this, ele->hypoIndex, tpIndex);
		queue.push(newEle);

		SetSeenPosition(ele->hypoIndex, tpIndex, seenPositions);
	}
}

void CubeEdge::Finalize()
{
}

}


