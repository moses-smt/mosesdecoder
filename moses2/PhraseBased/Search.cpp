/*
 * Search.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#include "Search.h"
#include "Manager.h"
#include "../System.h"
#include "../legacy/Bitmap.h"
#include "../legacy/Range.h"

namespace Moses2
{

Search::Search(Manager &mgr) :
  mgr(mgr)
{
  // TODO Auto-generated constructor stub

}

Search::~Search()
{
  // TODO Auto-generated destructor stub
}

bool Search::CanExtend(const Bitmap &hypoBitmap, size_t hypoRangeEndPos,
                       const Range &pathRange)
{
  const size_t hypoFirstGapPos = hypoBitmap.GetFirstGapPos();

  //cerr << "DOING " << hypoBitmap << " [" << hypoRange.GetStartPos() << " " << hypoRange.GetEndPos() << "]"
  //		  " [" << pathRange.GetStartPos() << " " << pathRange.GetEndPos() << "]";

  if (hypoBitmap.Overlap(pathRange)) {
    //cerr << " NO" << endl;
    return false;
  }

  if (mgr.system.options.reordering.max_distortion == -1) {
    return true;
  }

  if (mgr.system.options.reordering.max_distortion >= 0) {
    // distortion limit
    int distortion = ComputeDistortionDistance(hypoRangeEndPos,
                     pathRange.GetStartPos());
    if (distortion > mgr.system.options.reordering.max_distortion) {
      //cerr << " NO" << endl;
      return false;
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

  size_t closestRight = hypoBitmap.GetEdgeToTheRightOf(pathRange.GetEndPos());
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

    if (ComputeDistortionDistance(pathRange.GetEndPos(),
                                  bestNextExtension.GetStartPos()) > mgr.system.options.reordering.max_distortion) {
      //cerr << " NO" << endl;
      return false;
    }

    // everything is fine, we're good to go
  }

  return true;
}

}

