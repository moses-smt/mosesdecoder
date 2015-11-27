/*
 * CubePruning.cpp
 *
 *  Created on: 27 Nov 2015
 *      Author: hieu
 */

#include "CubePruning.h"
#include "Manager.h"

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

////////////////////////////////////////////////////////////////////////
void CubeElement::CreateHypothesis(Manager &mgr)
{
	const Hypothesis &prevHypo = *edge.hypos[hypoIndex];
	const TargetPhrase &tp = edge.tps[tpIndex];

	hypo = Hypothesis::Create(mgr);
	hypo->Init(prevHypo, tp, edge.path.range, edge.newBitmap, edge.estimatedScore);
}
