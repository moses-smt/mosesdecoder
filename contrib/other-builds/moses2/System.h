/*
 * System.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
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
    mutable MemPool systemPool;
    mutable OutputCollector bestCollector;

    // moses.ini params
    size_t stackSize;
    int maxDistortion;
    size_t maxPhraseLength;
    int numThreads;

    SearchAlgorithm searchAlgorithm;
    size_t popLimit;

    std::string outputFilePath;
    size_t nbestSize;
    bool onlyDistinct;

    bool outputHypoScore;

	System(const Parameter &paramsArg);
	virtual ~System();

	MemPool &GetManagerPool() const;
	FactorCollection &GetVocab() const;

	Recycler<Hypothesis*> &GetHypoRecycler() const;
	ObjectPoolContiguous<Hypothesis*> &GetBatchForEval() const;
	Bitmaps &GetBitmaps() const;

protected:
  mutable FactorCollection m_vocab;
  mutable boost::thread_specific_ptr<MemPool> m_managerPool;
  mutable boost::thread_specific_ptr< Recycler<Hypothesis*> > m_hypoRecycler;
  mutable boost::thread_specific_ptr< ObjectPoolContiguous<Hypothesis*> > m_batchForEval;

  mutable boost::thread_specific_ptr<Bitmaps> m_bitmaps;

  void LoadWeights();
  void LoadMappings();

  void ini_performance_options();
};

}

