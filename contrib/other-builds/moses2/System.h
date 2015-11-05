/*
 * System.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include "FF/FeatureFunctions.h"
#include "Weights.h"
#include "MemPool.h"
#include "moses/FactorCollection.h"
#include "moses/Parameter.h"

class FeatureFunction;
class StatefulFeatureFunction;
class PhraseTable;

class System {
public:
	System(const Moses::Parameter &paramsArg);
	virtual ~System();

	MemPool &GetManagerPool() const
	{ return m_managerPool; }

    const Moses::Parameter &params;
    mutable Moses::FactorCollection vocab;
    FeatureFunctions featureFunctions;
    Weights weights;
    std::vector<const PhraseTable*> mappings;

    MemPool systemPool;

    size_t stackSize;
protected:

  mutable MemPool m_managerPool;

  void LoadWeights();
  void LoadMappings();
};

