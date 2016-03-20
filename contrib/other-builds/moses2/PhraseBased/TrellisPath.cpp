/*
 * TrellisPath.cpp
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */
#include <cassert>
#include "TrellisPath.h"
#include "TrellisPaths.h"
#include "Hypothesis.h"
#include "../System.h"

using namespace std;

namespace Moses2 {

std::ostream& operator<<(std::ostream &out, const TrellishNode &node)
{
	out << "arcList=" << node.arcList->size() << " " << node.ind;
	return out;
}

/////////////////////////////////////////////////////////////////////////////////
TrellisPath::TrellisPath(const Hypothesis *hypo, const ArcLists &arcLists)
:prevEdgeChanged(-1)
{
	AddNodes(hypo, arcLists);
	m_scores = &hypo->GetScores();
}

TrellisPath::TrellisPath(const TrellisPath &origPath,
		size_t edgeIndex,
		const TrellishNode &newNode,
		const ArcLists &arcLists,
		MemPool &pool,
		const System &system)
:prevEdgeChanged(edgeIndex)
{
  nodes.reserve(origPath.nodes.size());
  for (size_t currEdge = 0 ; currEdge < edgeIndex ; currEdge++) {
	// copy path from parent
	nodes.push_back(origPath.nodes[currEdge]);
  }

  // 1 deviation
  nodes.push_back(newNode);

  // rest of path comes from following best path backwards
  const ArcList &arcList = *newNode.arcList;
  const Hypothesis *arc = static_cast<const Hypothesis*>(arcList[newNode.ind]);

  const Hypothesis *prevHypo = arc->GetPrevHypo();
  while (prevHypo != NULL) {
	const ArcList *arcList = arcLists.GetArcList(prevHypo);
	assert(arcList);
	TrellishNode node(*arcList, 0);
    nodes.push_back(node);

    prevHypo = prevHypo->GetPrevHypo();
  }

  CalcScores(origPath.GetScores(), pool, system);
}

TrellisPath::~TrellisPath() {
	// TODO Auto-generated destructor stub
}

SCORE TrellisPath::GetFutureScore() const
{
	return m_scores->GetTotalScore();
}

void TrellisPath::AddNodes(const Hypothesis *hypo, const ArcLists &arcLists)
{
	if (hypo) {
		// add this hypo
		//cerr << "hypo=" << hypo << " " << flush;
		//cerr << *hypo << endl;
		const ArcList *list = arcLists.GetArcList(hypo);
		assert(list);
		TrellishNode node(*list, 0);
		nodes.push_back(node);

		// add prev hypos
		const Hypothesis *prev = hypo->GetPrevHypo();
		AddNodes(prev, arcLists);
}
}

void TrellisPath::OutputToStream(std::ostream &out, const System &system) const
{
	//cerr << "path=" << this << " " << nodes.size() << endl;
	for (int i = nodes.size() - 1; i >= 0; --i) {
		const TrellishNode &node = nodes[i];
		const Hypothesis *hypo = static_cast<const Hypothesis*>((*node.arcList)[node.ind]);
		//cerr << "hypo=" << hypo << " " << *hypo << endl;
		hypo->GetTargetPhrase().OutputToStream(out);
		out << " ";
	}
	out << "||| ";

	GetScores().OutputToStream(out, system);
}

void TrellisPath::CreateDeviantPaths(TrellisPaths &paths,
		const ArcLists &arcLists,
		MemPool &pool,
		const System &system) const
{
  const size_t sizePath = nodes.size();

  cerr << "prevEdgeChanged=" << prevEdgeChanged << endl;
  for (size_t currEdge = prevEdgeChanged + 1 ; currEdge < sizePath ; currEdge++) {
	TrellishNode newNode = nodes[currEdge];
    assert(newNode.ind == 0);
	const ArcList &arcList = *newNode.arcList;

    cerr << "arcList=" << arcList.size() << endl;
	for (size_t i = 1; i < arcList.size(); ++i) {
		cerr << "i=" << i << endl;
		newNode.ind = i;

		TrellisPath *deviantPath = new TrellisPath(*this, currEdge, newNode, arcLists, pool, system);
		cerr << "deviantPath=" << deviantPath << endl;
		paths.Add(deviantPath);
	}
  }
}

void TrellisPath::CalcScores(const Scores &origScores, MemPool &pool, const System &system)
{
	Scores *scores = new (pool.Allocate<Scores>()) Scores(system, pool, system.featureFunctions.GetNumScores());
	m_scores = scores;
}

} /* namespace Moses2 */
