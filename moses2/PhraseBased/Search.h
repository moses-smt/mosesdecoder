/*
 * Search.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <stddef.h>
#include "../legacy/Util2.h"

namespace Moses2
{

class Manager;
class Stack;
class Hypothesis;
class Bitmap;
class Range;
class TrellisPath;

template<typename T>
class TrellisPaths;

class Search
{
public:
  Search(Manager &mgr);
  virtual ~Search();

  virtual void Decode() = 0;
  virtual const Hypothesis *GetBestHypo() const = 0;

  virtual void AddInitialTrellisPaths(TrellisPaths<TrellisPath> &paths) const = 0;

protected:
  Manager &mgr;
  //ArcLists m_arcLists;

  bool CanExtend(const Bitmap &hypoBitmap, size_t hypoRangeEndPos,
                 const Range &pathRange);

  inline int ComputeDistortionDistance(size_t prevEndPos,
                                       size_t currStartPos) const {
    int dist = 0;
    if (prevEndPos == NOT_FOUND) {
      dist = currStartPos;
    } else {
      dist = (int)prevEndPos - (int)currStartPos + 1;
    }
    return abs(dist);
  }

};

}

