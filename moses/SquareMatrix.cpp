// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <string>
#include <iostream>
#include "SquareMatrix.h"
#include "TypeDef.h"
#include "Util.h"

using namespace std;

namespace Moses
{
void SquareMatrix::InitTriangle(float val)
{
  for(size_t row=0; row < m_size; row++) {
    for(size_t col=row; col<m_size; col++) {
      SetScore(row, col, -numeric_limits<float>::infinity());
    }
  }
}

/**
 * Calculate future score estimate for a given coverage bitmap
 *
 * /param bitmap coverage bitmap
 */

float SquareMatrix::CalcEstimatedScore( Bitmap const &bitmap ) const
{
  const size_t notInGap= numeric_limits<size_t>::max();
  size_t startGap = notInGap;
  float estimatedScore = 0.0f;
  for(size_t currPos = 0 ; currPos < bitmap.GetSize() ; currPos++) {
    // start of a new gap?
    if(bitmap.GetValue(currPos) == false && startGap == notInGap) {
      startGap = currPos;
    }
    // end of a gap?
    else if(bitmap.GetValue(currPos) == true && startGap != notInGap) {
      estimatedScore += GetScore(startGap, currPos - 1);
      startGap = notInGap;
    }
  }
  // coverage ending with gap?
  if (startGap != notInGap) {
    estimatedScore += GetScore(startGap, bitmap.GetSize() - 1);
  }

  return estimatedScore;
}

/**
 * Calculare future score estimate for a given coverage bitmap
 * and an additional span that is also covered. This function is used
 * to compute future score estimates for hypotheses that we may want
 * build, but first want to check.
 *
 * Note: this function is implemented a bit more complex than
 * the basic one (w/o additional phrase) for speed reasons,
 * which is probably overkill.
 *
 * /param bitmap coverage bitmap
 * /param startPos start of the span that is added to the coverage
 * /param endPos end of the span that is added to the coverage
 */

float SquareMatrix::CalcEstimatedScore( Bitmap const &bitmap, size_t startPos, size_t endPos ) const
{
  const size_t notInGap= numeric_limits<size_t>::max();
  float estimatedScore = 0.0f;
  size_t startGap = bitmap.GetFirstGapPos();
  if (startGap == NOT_FOUND) return estimatedScore; // everything filled

  // start loop at first gap
  size_t startLoop = startGap+1;
  if (startPos == startGap) { // unless covered by phrase
    startGap = notInGap;
    startLoop = endPos+1; // -> postpone start
  }

  size_t lastCovered = bitmap.GetLastPos();
  if (endPos > lastCovered || lastCovered == NOT_FOUND) lastCovered = endPos;

  for(size_t currPos = startLoop; currPos <= lastCovered ; currPos++) {
    // start of a new gap?
    if(startGap == notInGap && bitmap.GetValue(currPos) == false && (currPos < startPos || currPos > endPos)) {
      startGap = currPos;
    }
    // end of a gap?
    else if(startGap != notInGap && (bitmap.GetValue(currPos) == true || (startPos <= currPos && currPos <= endPos))) {
      estimatedScore += GetScore(startGap, currPos - 1);
      startGap = notInGap;
    }
  }
  // coverage ending with gap?
  if (lastCovered != bitmap.GetSize() - 1) {
    estimatedScore += GetScore(lastCovered+1, bitmap.GetSize() - 1);
  }

  return estimatedScore;
}

TO_STRING_BODY(SquareMatrix);

}


