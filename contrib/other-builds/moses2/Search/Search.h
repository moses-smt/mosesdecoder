/*
 * Search.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#ifndef SEARCH_SEARCH_H_
#define SEARCH_SEARCH_H_

#include <stddef.h>

namespace Moses2
{

class Manager;
class Stack;
class Hypothesis;
class Bitmap;
class Range;

class Search {
public:
	Search(Manager &mgr);
	virtual ~Search();

	virtual void Decode() = 0;
	virtual const Hypothesis *GetBestHypothesis() const = 0;

protected:
	Manager &m_mgr;
	//ArcLists m_arcLists;

	bool CanExtend(const Bitmap &hypoBitmap, size_t hypoRangeEndPos, const Range &pathRange);
	int ComputeDistortionDistance(size_t prevEndPos, size_t currStartPos) const;

};

}


#endif /* SEARCH_SEARCH_H_ */
