/*
 * Search.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#ifndef SEARCH_SEARCH_H_
#define SEARCH_SEARCH_H_

#include <vector>

class Hypothesis;

class Search {
public:
	Search();
	virtual ~Search();

	virtual void Decode(size_t stackInd) = 0;

	virtual const Hypothesis *GetBestHypothesis() const = 0;

};

#endif /* SEARCH_SEARCH_H_ */
