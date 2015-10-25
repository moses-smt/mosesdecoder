/*
 * SearchNormal.cpp
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#include "SearchNormal.h"
#include "Stack.h"
#include "Manager.h"
#include "InputPaths.h"
#include "moses/Bitmap.h"

SearchNormal::SearchNormal(Manager &mgr, std::vector<Stack> &stacks)
:m_mgr(mgr)
,m_stacks(stacks)
{
	// TODO Auto-generated constructor stub

}

SearchNormal::~SearchNormal() {
	// TODO Auto-generated destructor stub
}

void SearchNormal::Decode(size_t stackInd)
{
	Stack &stack = m_stacks[stackInd];

	Stack::const_iterator iter;
	for (iter = stack.begin(); iter != stack.end(); ++iter) {
		const Hypothesis &hypo = **iter;
		Extend(hypo);
	}
}

void SearchNormal::Extend(const Hypothesis &hypo)
{
	const InputPaths &paths = m_mgr.GetInputPaths();
	InputPaths::const_iterator iter;
	for (iter = paths.begin(); iter != paths.end(); ++iter) {
		const InputPath &path = *iter;
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
	const std::vector<const TargetPhrases*> &tpsAllPt = path.GetTargetPhrases();

	for (size_t i = 0; i < tpsAllPt.size(); ++i) {
		const TargetPhrases *tps = tpsAllPt[i];
		if (tps) {
			Extend(hypo, *tps);
		}
	}
}

void SearchNormal::Extend(const Hypothesis &hypo, const TargetPhrases &tps)
{
	//Hypothesis *newHypo = new

}
