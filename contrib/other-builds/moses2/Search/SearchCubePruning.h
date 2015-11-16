/*
 * SearchCubePruning.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#ifndef SEARCH_SEARCHCUBEPRUNING_H_
#define SEARCH_SEARCHCUBEPRUNING_H_

#include "Search.h"

class SearchCubePruning : public Search
{
public:
	SearchCubePruning();
	virtual ~SearchCubePruning();

	void Decode(size_t stackInd);

	const Hypothesis *GetBestHypothesis() const;

};

#endif /* SEARCH_SEARCHCUBEPRUNING_H_ */
