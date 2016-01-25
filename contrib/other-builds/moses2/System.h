/*
 * System.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include <deque>
#include <boost/thread/tss.hpp>
#include <boost/pool/object_pool.hpp>
#include "FF/FeatureFunctions.h"
#include "Weights.h"
#include "MemPool.h"
#include "Recycler.h"
#include "legacy/FactorCollection.h"
#include "legacy/Parameter.h"
#include "TypeDef.h"
#include "Search/CubePruning/Misc.h"
#include "legacy/Bitmaps.h"
#include "legacy/OutputCollector.h"

namespace Moses2
{
	namespace NSCubePruning
	{
		class Stack;
	}

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
    mutable OutputCollector bestCollector;

    // moses.ini params
    size_t stackSize;
    int maxDistortion;
    size_t maxPhraseLength;
    int numThreads;

    SearchAlgorithm searchAlgorithm;
    size_t popLimit;
    size_t  cubePruningDiversity;
    bool cubePruningLazyScoring;

    std::string outputFilePath;
    size_t nbestSize;
    bool onlyDistinct;

    bool outputHypoScore;
    int reportSegmentation;
    int cpuAffinityOffset;
    int cpuAffinityOffsetIncr;

	System(const Parameter &paramsArg);
	virtual ~System();

	MemPool &GetSystemPool() const;
	MemPool &GetManagerPool() const;
	FactorCollection &GetVocab() const;

	Recycler<Hypothesis*> &GetHypoRecycler() const;
	ObjectPoolContiguous<Hypothesis*> &GetBatchForEval() const;

protected:
  mutable FactorCollection m_vocab;
  mutable boost::thread_specific_ptr<MemPool> m_managerPool;
  mutable boost::thread_specific_ptr<MemPool> m_systemPool;

  mutable boost::thread_specific_ptr< Recycler<Hypothesis*> > m_hypoRecycler;
  mutable boost::thread_specific_ptr< ObjectPoolContiguous<Hypothesis*> > m_batchForEval;

  void LoadWeights();
  void LoadMappings();

  void ini_performance_options();
};

}

