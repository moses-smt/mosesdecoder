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
#include "TypeDef.h"

class FeatureFunction;
class StatefulFeatureFunction;
class PhraseTable;
class Hypothesis;

class System {
public:
    const Parameter &params;
    FeatureFunctions featureFunctions;
    Weights weights;
    std::vector<const PhraseTable*> mappings;
    mutable MemPool systemPool;

    size_t stackSize;
    int maxDistortion;
    size_t maxPhraseLength;
    int numThreads;
    SearchAlgorithm searchAlgorithm;

	System(const Parameter &paramsArg);
	virtual ~System();

	MemPool &GetManagerPool() const;
	FactorCollection &GetVocab() const;

	ObjectPoolContiguous<Hypothesis*> &GetBatchForEval() const;

protected:
  mutable FactorCollection m_vocab;
  mutable boost::thread_specific_ptr<MemPool> m_managerPool;
  mutable boost::thread_specific_ptr< ObjectPoolContiguous<Hypothesis*> > m_batchForEval;

  void LoadWeights();
  void LoadMappings();

  void ini_performance_options();
};

