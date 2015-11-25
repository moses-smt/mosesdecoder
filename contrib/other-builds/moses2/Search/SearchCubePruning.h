/*
 * SearchCubePruning.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <boost/unordered_map.hpp>
#include <vector>
#include "Search.h"

class Bitmap;
class Hypothesis;

class SearchCubePruning : public Search
{
public:
	SearchCubePruning(Manager &mgr, Stacks &stacks);
	virtual ~SearchCubePruning();

	void Decode(size_t stackInd);

	const Hypothesis *GetBestHypothesis() const;

protected:
	boost::unordered_map<const Bitmap*, std::vector<const Hypothesis*> > m_hyposPerBM;

};

