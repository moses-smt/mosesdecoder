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

SearchNormal::SearchNormal(Manager &mgr, Stacks &stacks)
:m_mgr(mgr)
,m_stacks(stacks)
,m_stackSize(mgr.system.stackSize)
{
	// TODO Auto-generated constructor stub

}

SearchNormal::~SearchNormal() {
	// TODO Auto-generated destructor stub
}

void SearchNormal::Decode(size_t stackInd)
{
  Stack &stack = m_stacks[stackInd];

  std::vector<const Hypothesis*> hypos = stack.GetBestHyposAndPrune(m_stackSize, m_mgr.GetHypoRecycle());
  BOOST_FOREACH(const Hypothesis *hypo, hypos) {
		Extend(*hypo);
  }

  //cerr << m_stacks << endl;

  // delete stack to save mem
  m_stacks.Delete(stackInd);
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
	const Moses::Range &pathRange = path.range;

  const size_t hypoFirstGapPos = bitmap.GetFirstGapPos();

	if (bitmap.Overlap(pathRange)) {
		return;
	}

	if (m_mgr.system.maxDistortion >= 0) {
		// distortion limit
		int distortion = ComputeDistortionDistance(hypoRange, pathRange);
		if (distortion > m_mgr.system.maxDistortion) {
			return;
		}
	}


    // first question: is there a path from the closest translated word to the left
    // of the hypothesized extension to the start of the hypothesized extension?
    // long version:
    // - is there anything to our left?
    // - is it farther left than where we're starting anyway?
    // - can we get to it?

    // closestLeft is exclusive: a value of 3 means 2 is covered, our
    // arc is currently ENDING at 3 and can start at 3 implicitly

	// TODO is this relevant? only for lattice input?

    // ask second question here: we already know we can get to our
    // starting point from the closest thing to the left. We now ask the
    // follow up: can we get from our end to the closest thing on the
    // right?
    //
    // long version: is anything to our right? is it farther
    // right than our (inclusive) end? can our end reach it?
    bool isLeftMostEdge = (hypoFirstGapPos == pathRange.GetStartPos());

    size_t closestRight = bitmap.GetEdgeToTheRightOf(pathRange.GetEndPos());
    /*
    if (isWordLattice) {
      if (closestRight != endPos
          && ((closestRight + 1) < sourceSize)
          && !m_source.CanIGetFromAToB(endPos + 1, closestRight + 1)) {
        continue;
      }
    }
	*/

    if (isLeftMostEdge) {
      // any length extension is okay if starting at left-most edge

    } else { // starting somewhere other than left-most edge, use caution
      // the basic idea is this: we would like to translate a phrase
      // starting from a position further right than the left-most
      // open gap. The distortion penalty for the following phrase
      // will be computed relative to the ending position of the
      // current extension, so we ask now what its maximum value will
      // be (which will always be the value of the hypothesis starting
      // at the left-most edge).  If this value is less than the
      // distortion limit, we don't allow this extension to be made.
      Moses::Range bestNextExtension(hypoFirstGapPos, hypoFirstGapPos);

      if (ComputeDistortionDistance(pathRange, bestNextExtension)
          > m_mgr.system.maxDistortion) {
    	  return;
      }

      // everything is fine, we're good to go
    }


    // extend this hypo
	const Moses::Bitmap &newBitmap = m_mgr.GetBitmaps().GetBitmap(bitmap, pathRange);

	  cerr << "DOING " << bitmap << " [" << hypoRange.GetStartPos() << " " << hypoRange.GetEndPos() << "]"
			  " [" << pathRange.GetStartPos() << " " << pathRange.GetEndPos() << "]" << endl;

	const std::vector<TargetPhrases::shared_const_ptr> &tpsAllPt = path.targetPhrases;

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
	Hypothesis *newHypo = Hypothesis::Create(m_mgr);
	newHypo->Init(hypo, tp, pathRange, newBitmap);
	newHypo->EvaluateWhenApplied();

	size_t numWordsCovered = newBitmap.GetNumWordsCovered();
	Stack &stack = m_stacks[numWordsCovered];
	StackAdd added = stack.Add(newHypo);

	Recycler<Hypothesis*> &hypoRecycle = m_mgr.GetHypoRecycle();

	if (added.added) {
		// we're winners!
		if (added.other) {
			// there was a existing losing hypo
			hypoRecycle.push(added.other);
		}
	}
	else {
		// we're losers!
		// there should be a winner, we're not doing beam pruning
		UTIL_THROW_IF2(added.other == NULL, "There must have been a winning hypo");
		hypoRecycle.push(newHypo);
	}

	//m_arcLists.AddArc(stackAdded.added, newHypo, stackAdded.other);
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

int SearchNormal::ComputeDistortionDistance(const Moses::Range& prev, const Moses::Range& current) const
{
  int dist = 0;
  if (prev.GetNumWordsCovered() == 0) {
    dist = current.GetStartPos();
  } else {
    dist = (int)prev.GetEndPos() - (int)current.GetStartPos() + 1 ;
  }
  return abs(dist);
}

