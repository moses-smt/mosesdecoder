/*
 * System.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include "Vocab.h"
#include "Weights.h"
#include "util/pool.hh"

class FeatureFunction;
class StatefulFeatureFunction;
class PhraseTable;

class System {
public:
	System();
	virtual ~System();

	size_t GetNumScores() const
	{ return 55; }

	const Weights &GetWeights() const
	{ return m_weights; }

	util::Pool &GetPool()
	{ return m_pool; }

	const std::vector<const PhraseTable*> &GetPhraseTables() const
	{ return m_phraseTables; }

	const std::vector<const StatefulFeatureFunction*> &GetStatefulFeatureFunctions() const
	{ return m_statefulFeatureFunctions; }

protected:
  Vocab m_vocab;
  std::vector<const FeatureFunction*> m_featureFunctions;
  std::vector<const StatefulFeatureFunction*> m_statefulFeatureFunctions;
  std::vector<const PhraseTable*> m_phraseTables;
  util::Pool m_pool;
  size_t m_ffStartInd;
  Weights m_weights;
};

