/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <boost/shared_ptr.hpp>

#include "Utils.h"

#include "Gain.h"
#include "OnlineLearner.h"
#include "Sampler.h"
#include "Selector.h"

namespace Josiah {

/** Calculates the gain due to a delta.
  **/
class DeltaGain {
  public:
    DeltaGain(const GainFunctionHandle& gainFunction, const SampleVector& samples, size_t sampleId);
    float operator()(const TDeltaHandle& delta) ;
    
  private:
    const GainFunctionHandle& m_gainFunction;
    const SampleVector& m_samples;
    size_t m_sampleId;
    //cached list of translations
    std::vector<Translation> m_translations;
};


/**
  * Used to choose the oracle translation hypothesis.
 **/
class TargetAssigner {
  public:
    TargetAssigner(const std::string& name) : m_name(name) {}
    virtual ~TargetAssigner(){}
    size_t virtual getTarget(const TDeltaVector& deltas, const TDeltaHandle& noChangeDelta,
                             DeltaGain& gf) = 0;
    std::string m_name;
};

class BestNeighbourTgtAssigner : public TargetAssigner {
  public:
    BestNeighbourTgtAssigner() : TargetAssigner("Best") {}
    virtual ~BestNeighbourTgtAssigner(){}
    size_t virtual getTarget(const TDeltaVector& deltas, const TDeltaHandle& noChangeDelta,
                             DeltaGain& gf);
};

class ClosestBestNeighbourTgtAssigner : public TargetAssigner {
  public:
    ClosestBestNeighbourTgtAssigner(): TargetAssigner("CBN") {}
    virtual ~ClosestBestNeighbourTgtAssigner(){}
    size_t virtual getTarget(const TDeltaVector& deltas, const TDeltaHandle& noChangeDelta,
                             DeltaGain& gf);
};

class ChiangBestNeighbourTgtAssigner : public TargetAssigner {
  public:
    ChiangBestNeighbourTgtAssigner(): TargetAssigner("Chiang"){}
    virtual ~ChiangBestNeighbourTgtAssigner(){}
    size_t virtual getTarget(const TDeltaVector& deltas, const TDeltaHandle& noChangeDelta,
                             DeltaGain& gf);
};

typedef boost::shared_ptr<TargetAssigner> TargetAssignerHandle;

/** 
 * In charge of collecting the weights and writing them to file.
 **/
class WeightCollector {
  public:
    /** 
      * If frequency is non-zero, then dump weights to file. Count
      * by samples, or by batch.
      **/
    WeightCollector(size_t frequency, bool byBatch,
         const std::string& weightDumpStem, size_t size, size_t rank);
    void updateWeights();
    void endBatch();
    FVector getAverageWeights();
    size_t getBatchCount();
    void SetL1Normalise(bool l1normalise);
    void SetLag(size_t lag);
    void SetDumpCurrent(bool dumpCurrent);


  private:
    size_t m_frequency;
    bool m_byBatch;
    std::string m_weightDumpStem;
    FVector m_totalWeights;
    size_t m_updates;
    size_t m_allUpdates;
    size_t m_batches;
    size_t m_size;
    size_t m_rank;
    bool m_l1normalise;
    std::string m_unknownWordPenaltyName;
    size_t m_lag;
    //dump current weights instead of average
    bool m_dumpCurrent;
    
};

typedef boost::shared_ptr<WeightCollector> WeightCollectorHandle;

/**
 * Implements the Sample Rank algorithm, by accepting the proposed list of deltas, choosing one,
 * updating the weights accordingly, and returning the delta to apply.
 **/
class SampleRankSelector : public DeltaSelector {
  
  public:
    SampleRankSelector(const GainFunctionHandle& gainFunction, 
                       const OnlineLearnerHandle& onlineLearner, 
                       const TargetAssignerHandle& assigner,
                       const WeightCollectorHandle& weightCollector);
    
    /** Body of SampleRank algorithm */
    virtual TDeltaHandle Select(size_t sampleId,
                                const TDeltaVector& deltas,
                                const TDeltaHandle& noChangeDelta, 
                                size_t iteration);
    virtual void BeginBurnin();
    virtual void EndBurnin();
    virtual void SetSamples(const SampleVector& samples);
    void SetTemperature(float temp);
    void SetBurninAnnealer(AnnealingSchedule* schedule);
    void SetIgnoreUnknownWordPenalty(bool ignore);
    void SetTolerance(float tolerance);
    void SetAlwaysUpdate(bool alwaysUpdate);
    void SetUpdateTarget(bool updateTarget);
    virtual ~SampleRankSelector() {}
    
  private:
    void UpdateGainOptimalSol(
        const TDeltaVector& deltas, 
        const TDeltaHandle& noChangeDelta,
        size_t sampleId,
        DeltaGain& gain,
        int target);
    
    const GainFunctionHandle m_gainFunction;
    const OnlineLearnerHandle m_onlineLearner;
    const TargetAssignerHandle m_assigner;
    const WeightCollectorHandle m_weightCollector;
    bool m_burnin;
    SamplingSelector m_samplingSelector;
    SamplingSelector m_burninSamplingSelector;
    //feature values for optimal gain solution
    std::vector<FVector> m_optimalGainSolutionScores;
    //gain of optimal gain solution
    std::vector<FValue> m_optimalGain;
    //The current batch of samples
    SampleVector m_samples;
    bool m_ignoreUnknownWordPenalty;
    std::string m_unknownWordPenaltyName;
    //difference between chosen bleu and target bleu must be greater
    //than this to force a weight update
    float m_tolerance;
    //always call the updater, even if the ranking is correct. I think
    //this is what Aron Culotta does.
    bool m_alwaysUpdate;
    //Jump to the target instead of the chosen
    bool m_updateTarget;
    
};



}

