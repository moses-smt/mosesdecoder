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

using namespace std;

namespace Moses2 {

std::ostream& operator<<(std::ostream &out, const TrellishNode &node)
{
	out << "arcList=" << node.arcList.size() << " " << node.ind;
	return out;
}

/////////////////////////////////////////////////////////////////////////////////
TrellisPath::TrellisPath(const Hypothesis *hypo, const ArcLists &arcLists)
:prevEdgeChanged(-1)
{
	AddNodes(hypo, arcLists);
	m_scores = &hypo->GetScores();
}

TrellisPath::TrellisPath(const TrellisPath &origPath, size_t edgeIndex, const Hypothesis *arc)
{

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
		TrellishNode *node = new TrellishNode(*list, 0);
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
		const TrellishNode *node = nodes[i];
		const Hypothesis *hypo = static_cast<const Hypothesis*>(node->arcList[node->ind]);
		//cerr << "hypo=" << hypo << " " << *hypo << endl;
		hypo->GetTargetPhrase().OutputToStream(out);
		out << " ";
	}
	out << "||| ";

	GetScores().OutputToStream(out, system);
}

void TrellisPath::CreateDeviantPaths(TrellisPaths &paths) const
{
  const size_t sizePath = nodes.size();

  for (size_t currEdge = prevEdgeChanged + 1 ; currEdge < sizePath ; currEdge++) {
	const TrellishNode &node = *nodes[currEdge];
    assert(node.ind == 0);
	const ArcList &arcList = node.arcList;

	for (size_t i = 1; i < arcList.size(); ++i) {
	      const Hypothesis *arcReplace = static_cast<const Hypothesis *>(arcList[i]);

	      TrellisPath *deviantPath = new TrellisPath(*this, currEdge, arcReplace);
	      paths.Add(deviantPath);
	}
  }
}

} /* namespace Moses2 */
