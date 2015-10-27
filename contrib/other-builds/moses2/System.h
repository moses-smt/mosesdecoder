/*
 * System.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include "Weights.h"
#include "util/pool.hh"
#include "moses/FactorCollection.h"
#include "moses/Parameter.h"

class FeatureFunction;
class StatefulFeatureFunction;
class PhraseTable;

class System {
public:
	System(const Moses::Parameter &params);
	virtual ~System();

	size_t GetNumScores() const
	{ return 55; }

	size_t GetFFStartInd() const
	{ return m_ffStartInd; }

	const Weights &GetWeights() const
	{ return m_weights; }

	util::Pool &GetSystemPool()
	{ return m_systemPool; }

	util::Pool &GetManagerPool()
	{ return m_managerPool; }

	const std::vector<const PhraseTable*> &GetPhraseTables() const
	{ return m_phraseTables; }

	const std::vector<const StatefulFeatureFunction*> &GetStatefulFeatureFunctions() const
	{ return m_statefulFeatureFunctions; }

	Moses::FactorCollection &GetVocab() const
	{ return m_vocab; }

protected:
  const Moses::Parameter &m_params;

  mutable Moses::FactorCollection m_vocab;
  std::vector<const FeatureFunction*> m_featureFunctions;
  std::vector<const StatefulFeatureFunction*> m_statefulFeatureFunctions;
  std::vector<const PhraseTable*> m_phraseTables;
  util::Pool m_systemPool;
  util::Pool m_managerPool;
  size_t m_ffStartInd;
  Weights m_weights;

  void LoadFeatureFunctions();
};

