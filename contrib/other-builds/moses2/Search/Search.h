/*
 * Search.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#ifndef SEARCH_SEARCH_H_
#define SEARCH_SEARCH_H_

#include <stddef.h>

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

	bool CanExtend(const Bitmap &hypoBitmap, const Range &hypoRange, const Range &pathRange);
	int ComputeDistortionDistance(const Range& prev, const Range& current) const;

};

#endif /* SEARCH_SEARCH_H_ */
