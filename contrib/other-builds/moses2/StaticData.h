/*
 * StaticData.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include "Vocab.h"
#include "util/pool.hh"

class FeatureFunction;

class StaticData {
public:
	StaticData();
	virtual ~StaticData();

	size_t GetNumScores() const
	{ return 55; }

	util::Pool &GetPool()
	{ return m_pool; }

protected:
  Vocab m_vocab;
  std::vector<FeatureFunction*> m_featureFunctions;
  util::Pool m_pool;
};

