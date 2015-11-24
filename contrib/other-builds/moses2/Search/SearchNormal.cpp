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
:Search(mgr, stacks)
{
	// TODO Auto-generated constructor stub

}

SearchNormal::~SearchNormal() {
	// TODO Auto-generated destructor stub
}

void SearchNormal::Decode(size_t stackInd)
{
  Stack &stack = m_stacks[stackInd];
  std::vector<const Hypothesis*> hypos = stack.GetBestHyposAndPrune(m_mgr.system.stackSize, m_mgr.GetHypoRecycle());

	const InputPaths &paths = m_mgr.GetInputPaths();

	BOOST_FOREACH(const InputPath &path, paths) {
		Extend(hypos, path);
	}

  //cerr << m_stacks << endl;

  // delete stack to save mem
  m_stacks.Delete(stackInd);
}

void SearchNormal::Extend(const std::vector<const Hypothesis*> &hypos, const InputPath &path)
{
	  BOOST_FOREACH(const Hypothesis *hypo, hypos) {
			Extend(*hypo, path);
	  }
}

void SearchNormal::Extend(const Hypothesis &hypo, const InputPath &path)
{
	const Bitmap &bitmap = hypo.GetBitmap();
	const Range &hypoRange = hypo.GetRange();
	const Range &pathRange = path.range;

    const size_t hypoFirstGapPos = bitmap.GetFirstGapPos();

    //cerr << "DOING " << bitmap << " [" << hypoRange.GetStartPos() << " " << hypoRange.GetEndPos() << "]"
	//		  " [" << pathRange.GetStartPos() << " " << pathRange.GetEndPos() << "]";

	if (bitmap.Overlap(pathRange)) {
		//cerr << " NO" << endl;
		return;
	}

	if (m_mgr.system.maxDistortion >= 0) {
		// distortion limit
		int distortion = ComputeDistortionDistance(hypoRange, pathRange);
		if (distortion > m_mgr.system.maxDistortion) {
			//cerr << " NO" << endl;
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
      Range bestNextExtension(hypoFirstGapPos, hypoFirstGapPos);

      if (ComputeDistortionDistance(pathRange, bestNextExtension)
          > m_mgr.system.maxDistortion) {
    	  //cerr << " NO" << endl;
    	  return;
      }

      // everything is fine, we're good to go
    }

	//cerr << " YES" << endl;

    // extend this hypo
	const Bitmap &newBitmap = m_mgr.GetBitmaps().GetBitmap(bitmap, pathRange);
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
  Hypothesis *arr[20];

  size_t numHypos = 0;
  BOOST_FOREACH(const TargetPhrase *tp, tps) {
		Hypothesis *newHypo = Hypothesis::Create(m_mgr);
		newHypo->Init(hypo, *tp, pathRange, newBitmap, estimatedScore);
		arr[numHypos] = newHypo;
		++numHypos;
  }

  for (size_t i = 0; i < numHypos; ++i) {
	  	Hypothesis *newHypo = arr[i];
		newHypo->EvaluateWhenApplied();
  }

  for (size_t i = 0; i < numHypos; ++i) {
	  	Hypothesis *newHypo = arr[i];
		size_t numWordsCovered = newBitmap.GetNumWordsCovered();
		Stack &stack = m_stacks[numWordsCovered];
		StackAdd added = stack.Add(newHypo);

		ObjectPoolContiguous<Hypothesis*> &hypoRecycle = m_mgr.GetHypoRecycle();

		if (added.toBeDeleted) {
			hypoRecycle.push(added.toBeDeleted);
		}

		//m_arcLists.AddArc(stackAdded.added, newHypo, stackAdded.other);
		//stack.Prune(m_mgr.GetHypoRecycle(), m_mgr.system.stackSize, m_mgr.system.stackSize * 2);
  }
}

/*
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
*/
void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrase &tp,
		const Range &pathRange,
		const Bitmap &newBitmap,
		SCORE estimatedScore)
{
	Hypothesis *newHypo = Hypothesis::Create(m_mgr);
	newHypo->Init(hypo, tp, pathRange, newBitmap, estimatedScore);
	newHypo->EvaluateWhenApplied();

	size_t numWordsCovered = newBitmap.GetNumWordsCovered();
	Stack &stack = m_stacks[numWordsCovered];
	StackAdd added = stack.Add(newHypo);

	ObjectPoolContiguous<Hypothesis*> &hypoRecycle = m_mgr.GetHypoRecycle();

	if (added.toBeDeleted) {
		hypoRecycle.push(added.toBeDeleted);
	}

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

int SearchNormal::ComputeDistortionDistance(const Range& prev, const Range& current) const
{
  int dist = 0;
  if (prev.GetNumWordsCovered() == 0) {
    dist = current.GetStartPos();
  } else {
    dist = (int)prev.GetEndPos() - (int)current.GetStartPos() + 1 ;
  }
  return abs(dist);
}

