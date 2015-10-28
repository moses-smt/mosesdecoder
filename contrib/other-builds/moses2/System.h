/*
 * System.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include "FeatureFunctions.h"
#include "Weights.h"
#include "MemPool.h"
#include "moses/FactorCollection.h"
#include "moses/Parameter.h"

class FeatureFunction;
class StatefulFeatureFunction;
class PhraseTable;

class System {
public:
	System(const Moses::Parameter &params);
	virtual ~System();

	const Weights &GetWeights() const
	{ return m_weights; }

	const FeatureFunctions &GetFeatureFunctions() const
	{ return m_featureFunctions; }

	MemPool &GetSystemPool()
	{ return m_systemPool; }

	MemPool &GetManagerPool()
	{ return m_managerPool; }

	Moses::FactorCollection &GetVocab() const
	{ return m_vocab; }

	const Moses::Parameter &GetParameter() const
	{ return m_params; }

protected:
  const Moses::Parameter &m_params;

  mutable Moses::FactorCollection m_vocab;
  MemPool m_systemPool;
  MemPool m_managerPool;
  FeatureFunctions m_featureFunctions;
  Weights m_weights;

  void LoadWeights();
};

