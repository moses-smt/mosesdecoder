/*
 * System.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include <boost/thread/tss.hpp>
#include "FF/FeatureFunctions.h"
#include "Weights.h"
#include "MemPool.h"
#include "Recycler.h"
#include "legacy/FactorCollection.h"
#include "legacy/Parameter.h"

class FeatureFunction;
class StatefulFeatureFunction;
class PhraseTable;
class Hypothesis;

class System {
public:
	System(const Parameter &paramsArg);
	virtual ~System();

	MemPool &GetManagerPool() const;
	Recycler<Hypothesis*> &GetHypoRecycle() const;

    const Parameter &params;
    mutable FactorCollection vocab;
    FeatureFunctions featureFunctions;
    Weights weights;
    std::vector<const PhraseTable*> mappings;

    MemPool systemPool;

    size_t stackSize;
    int maxDistortion;
    size_t maxPhraseLength;
    int numThreads;

protected:
  mutable boost::thread_specific_ptr<MemPool> m_managerPool;
  mutable boost::thread_specific_ptr<Recycler<Hypothesis*> > m_hypoRecycle;

  void LoadWeights();
  void LoadMappings();

  void ini_performance_options();
};

