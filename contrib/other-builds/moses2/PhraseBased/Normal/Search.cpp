/*
 * SearchNormal.cpp
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#include "Search.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include "Stack.h"
#include "../Manager.h"
#include "../../InputPaths.h"
#include "../../TargetPhrases.h"
#include "../../Phrase.h"
#include "../../System.h"
#include "../../Sentence.h"

using namespace std;

namespace Moses2
{

SearchNormal::SearchNormal(Manager &mgr)
:Search(mgr)
{
	// TODO Auto-generated constructor stub

}

SearchNormal::~SearchNormal() {
	// TODO Auto-generated destructor stub
}

void SearchNormal::Decode()
{
	// init stacks
	m_stacks.Init(mgr.GetInput().GetSize() + 1);

	const Bitmap &initBitmap = mgr.GetBitmaps().GetInitialBitmap();
	Hypothesis *initHypo = Hypothesis::Create(mgr.GetSystemPool(), mgr);
	initHypo->Init(mgr, mgr.GetInputPaths().GetBlank(), mgr.GetInitPhrase(), initBitmap);
	initHypo->EmptyHypothesisState(mgr.GetInput());

	m_stacks.Add(initHypo, mgr.GetHypoRecycle());

	for (size_t stackInd = 0; stackInd < m_stacks.GetSize(); ++stackInd) {
		Decode(stackInd);
		//cerr << m_stacks << endl;

		// delete stack to save mem
		if (stackInd < m_stacks.GetSize() - 1) {
			m_stacks.Delete(stackInd);
		}
		//cerr << m_stacks << endl;
	}
}

void SearchNormal::Decode(size_t stackInd)
{
  Stack &stack = m_stacks[stackInd];
  if (&stack == &m_stacks.Back()) {
	  // last stack. don't do anythin
	  return;
  }

  std::vector<const Hypothesis*> hypos = stack.GetBestHyposAndPrune(mgr.system.stackSize, mgr.GetHypoRecycle());

	const InputPaths &paths = mgr.GetInputPaths();

	BOOST_FOREACH(const InputPath *path, paths) {
	  BOOST_FOREACH(const Hypothesis *hypo, hypos) {
			Extend(*hypo, *path);
	  }
	}
}

void SearchNormal::Extend(const Hypothesis &hypo, const InputPath &path)
{
	const Bitmap &hypoBitmap = hypo.GetBitmap();
	const Range &hypoRange = hypo.GetInputPath().range;
	const Range &pathRange = path.range;

	if (!CanExtend(hypoBitmap, hypoRange.GetEndPos(), pathRange)) {
		return;
	}
	//cerr << " YES" << endl;

    // extend this hypo
	const Bitmap &newBitmap = mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
    //SCORE estimatedScore = mgr.GetEstimatedScores().CalcFutureScore2(bitmap, pathRange.GetStartPos(), pathRange.GetEndPos());
    SCORE estimatedScore = mgr.GetEstimatedScores().CalcEstimatedScore(newBitmap);

    size_t numPt = mgr.system.mappings.size();
	const TargetPhrases **tpsAllPt = path.targetPhrases;
	for (size_t i = 0; i < numPt; ++i) {
		const TargetPhrases *tps = tpsAllPt[i];
		if (tps) {
			Extend(hypo, *tps, path, newBitmap, estimatedScore);
		}
	}
}

void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrases &tps,
		const InputPath &path,
		const Bitmap &newBitmap,
		SCORE estimatedScore)
{
  BOOST_FOREACH(const TargetPhrase *tp, tps) {
	  Extend(hypo, *tp, path, newBitmap, estimatedScore);
  }
}

void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrase &tp,
		const InputPath &path,
		const Bitmap &newBitmap,
		SCORE estimatedScore)
{
	Hypothesis *newHypo = Hypothesis::Create(mgr.GetSystemPool(), mgr);
	newHypo->Init(mgr, hypo, path, tp, newBitmap, estimatedScore);
	newHypo->EvaluateWhenApplied();

	m_stacks.Add(newHypo, mgr.GetHypoRecycle());

	//m_arcLists.AddArc(stackAdded.added, newHypo, stackAdded.other);
	//stack.Prune(mgr.GetHypoRecycle(), mgr.system.stackSize, mgr.system.stackSize * 2);

}

const Hypothesis *SearchNormal::GetBestHypothesis() const
{
	const Stack &lastStack = m_stacks.Back();
	std::vector<const Hypothesis*> sortedHypos = lastStack.GetBestHypos(1);

	const Hypothesis *best = NULL;
	if (sortedHypos.size()) {
		best = sortedHypos[0];
	}
	return best;
}

}

