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

using namespace std;

SearchNormal::SearchNormal(Manager &mgr, std::vector<Stack> &stacks)
:m_mgr(mgr)
,m_stacks(stacks)
,m_stackSize(mgr.GetSystem().stackSize)
{
	// TODO Auto-generated constructor stub

}

SearchNormal::~SearchNormal() {
	// TODO Auto-generated destructor stub
}

void SearchNormal::Decode(size_t stackInd)
{
  Stack &stack = m_stacks[stackInd];

  std::vector<const Hypothesis*> hypos = stack.GetBestHypos(m_stackSize);
  BOOST_FOREACH(const Hypothesis *hypo, hypos) {
		Extend(*hypo);
  }
  DebugStacks();
}

void SearchNormal::Extend(const Hypothesis &hypo)
{
	const InputPaths &paths = m_mgr.GetInputPaths();

	BOOST_FOREACH(const InputPath &path, paths) {
		Extend(hypo, path);
	}
}

void SearchNormal::Extend(const Hypothesis &hypo, const InputPath &path)
{
	const Moses::Bitmap &bitmap = hypo.GetBitmap();
	const Moses::Range &hypoRange = hypo.GetRange();
	const Moses::Range &pathRange = path.GetRange();

	if (bitmap.Overlap(pathRange)) {
		return;
	}

	int distortion = abs((int)pathRange.GetStartPos() - (int)hypoRange.GetEndPos() - 1);
	if (distortion > 5) {
		return;
	}

	const Moses::Bitmap &newBitmap = m_mgr.GetBitmaps().GetBitmap(bitmap, pathRange);

	const std::vector<TargetPhrases::shared_const_ptr> &tpsAllPt = path.GetTargetPhrases();

	for (size_t i = 0; i < tpsAllPt.size(); ++i) {
		const TargetPhrases *tps = tpsAllPt[i].get();
		if (tps) {
			Extend(hypo, *tps, pathRange, newBitmap);
		}
	}
}

void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrases &tps,
		const Moses::Range &pathRange,
		const Moses::Bitmap &newBitmap)
{
  BOOST_FOREACH(const TargetPhrase *tp, tps) {
	  Extend(hypo, *tp, pathRange, newBitmap);
  }
}

void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrase &tp,
		const Moses::Range &pathRange,
		const Moses::Bitmap &newBitmap)
{
	Hypothesis *newHypo = new (m_mgr.GetPool().Allocate<Hypothesis>()) Hypothesis(hypo, tp, pathRange, newBitmap);
	newHypo->EvaluateWhenApplied();

	size_t numWordsCovered = newBitmap.GetNumWordsCovered();
	Stack &stack = m_stacks[numWordsCovered];
	StackAdd stackAdded = stack.Add(newHypo);

	m_arcLists.AddArc(stackAdded.added, newHypo, stackAdded.other);
}

void SearchNormal::DebugStacks() const
{
	  BOOST_FOREACH(const Stack &stack, m_stacks) {
		  cerr << stack.GetSize() << " ";
	  }
	  cerr << endl;
}

const Hypothesis *SearchNormal::GetBestHypothesis() const
{
	const Stack &lastStack = m_stacks.back();
	std::vector<const Hypothesis*> sortedHypos = lastStack.GetSortedHypos();

	const Hypothesis *best = NULL;
	if (sortedHypos.size()) {
		best = sortedHypos[0];
	}
	return best;
}


