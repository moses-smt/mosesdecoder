/*
 * CubePruning.cpp
 *
 *  Created on: 27 Nov 2015
 *      Author: hieu
 */

#include "CubePruning.h"
#include "Manager.h"

using namespace std;

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

////////////////////////////////////////////////////////////////////////
CubeElement::CubeElement(Manager &mgr, const CubeEdge &edge, size_t hypoIndex, size_t tpIndex)
:edge(edge)
,hypoIndex(hypoIndex)
,tpIndex(tpIndex)
{
	CreateHypothesis(mgr);
}

void CubeElement::CreateHypothesis(Manager &mgr)
{
	const Hypothesis *prevHypo = edge.hypos[hypoIndex];
	const TargetPhrase &tp = edge.tps[tpIndex];

	//cerr << "hypoIndex=" << hypoIndex << endl;
	//cerr << "edge.hypos=" << edge.hypos.size() << endl;
	//cerr << prevHypo << endl;
	//cerr << *prevHypo << endl;

	hypo = Hypothesis::Create(mgr);
	hypo->Init(*prevHypo, tp, edge.path.range, edge.newBitmap, edge.estimatedScore);
}
