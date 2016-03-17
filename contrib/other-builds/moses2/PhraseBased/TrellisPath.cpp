/*
 * TrellisPath.cpp
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */
#include <cassert>
#include "TrellisPath.h"
#include "Hypothesis.h"

using namespace std;

namespace Moses2 {

std::ostream& operator<<(std::ostream &out, const TrellishNode &node)
{
	out << "arcList=" << node.arcList.size() << " " << node.ind;
	return out;
}

TrellisPath::TrellisPath() {
	// TODO Auto-generated constructor stub

}

TrellisPath::TrellisPath(const Hypothesis *hypo, const ArcLists &arcLists)
{
	AddNodes(hypo, arcLists);
	m_scores = &hypo->GetScores();
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
		// add prev hypos
		const Hypothesis *prev = hypo->GetPrevHypo();
		AddNodes(prev, arcLists);

		// add this hypo
		//cerr << "hypo=" << hypo << " " << flush;
		//cerr << *hypo << endl;
		const ArcList *list = arcLists.GetArcList(hypo);
		assert(list);
		TrellishNode *node = new TrellishNode(*list, 0);
		nodes.push_back(node);
}
}

void TrellisPath::OutputToStream(std::ostream &out, const System &system) const
{
	cerr << "path=" << this << " " << nodes.size() << endl;
	for (size_t i = 0; i < nodes.size(); ++i) {
		const TrellishNode *node = nodes[i];
		const Hypothesis *hypo = static_cast<const Hypothesis*>(node->arcList[node->ind]);
		//cerr << "hypo=" << hypo << " " << *hypo << endl;
		hypo->GetTargetPhrase().OutputToStream(out);
		out << " ";
	}
	out << "||| ";

	GetScores().OutputToStream(out, system);
}

} /* namespace Moses2 */
