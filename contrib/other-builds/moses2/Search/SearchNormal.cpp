/*
 * SearchNormal.cpp
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#include <algorithm>
#include <boost/foreach.hpp>
#include "SearchNormal.h"
#include "Stack.h"
#include "Manager.h"
#include "../InputPaths.h"
#include "../TargetPhrases.h"
#include "../TargetPhrase.h"
#include "../System.h"
#include "../TranslationTask.h"

using namespace std;

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
	m_stacks.Init(m_mgr.GetInput().GetSize() + 1);

	const Bitmap &initBitmap = m_mgr.GetBitmaps().GetInitialBitmap();
	Hypothesis *initHypo = Hypothesis::Create(m_mgr);
	initHypo->Init(m_mgr.GetInitPhrase(), m_mgr.GetInitRange(), initBitmap);
	initHypo->EmptyHypothesisState(m_mgr.GetInput());

	m_stacks.Add(initHypo, m_mgr.GetHypoPool());

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

  std::vector<const Hypothesis*> hypos = stack.GetBestHyposAndPrune(m_mgr.system.stackSize, m_mgr.GetHypoPool());

	const InputPaths &paths = m_mgr.GetInputPaths();

	BOOST_FOREACH(const InputPath &path, paths) {
	  BOOST_FOREACH(const Hypothesis *hypo, hypos) {
			Extend(*hypo, path);
	  }
	}
}

void SearchNormal::Extend(const Hypothesis &hypo, const InputPath &path)
{
	const Bitmap &hypoBitmap = hypo.GetBitmap();
	const Range &hypoRange = hypo.GetRange();
	const Range &pathRange = path.range;

	if (!CanExtend(hypoBitmap, hypoRange.GetEndPos(), pathRange)) {
		return;
	}
	//cerr << " YES" << endl;

    // extend this hypo
	const Bitmap &newBitmap = m_mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
    //SCORE estimatedScore = m_mgr.GetEstimatedScores().CalcFutureScore2(bitmap, pathRange.GetStartPos(), pathRange.GetEndPos());
    SCORE estimatedScore = m_mgr.GetEstimatedScores().CalcEstimatedScore(newBitmap);

	const std::vector<TargetPhrases::shared_const_ptr> &tpsAllPt = path.targetPhrases;
	for (size_t i = 0; i < tpsAllPt.size(); ++i) {
		const TargetPhrases *tps = tpsAllPt[i].get();
		if (tps) {
			Extend(hypo, *tps, pathRange, newBitmap, estimatedScore);
		}
	}
}

void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrases &tps,
		const Range &pathRange,
		const Bitmap &newBitmap,
		SCORE estimatedScore)
{
  BOOST_FOREACH(const TargetPhrase *tp, tps) {
	  Extend(hypo, *tp, pathRange, newBitmap, estimatedScore);
  }
}

void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrase &tp,
		const Range &pathRange,
		const Bitmap &newBitmap,
		SCORE estimatedScore)
{
	Hypothesis *newHypo = Hypothesis::Create(m_mgr);
	newHypo->Init(hypo, tp, pathRange, newBitmap, estimatedScore);
	newHypo->EvaluateWhenApplied();

	m_stacks.Add(newHypo, m_mgr.GetHypoPool());

	//m_arcLists.AddArc(stackAdded.added, newHypo, stackAdded.other);
	//stack.Prune(m_mgr.GetHypoRecycle(), m_mgr.system.stackSize, m_mgr.system.stackSize * 2);

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

