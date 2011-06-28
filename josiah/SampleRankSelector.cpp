/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 University of Edinburgh
 
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

#include "SampleRankSelector.h"

#ifdef MPI_ENABLED
#include <mpi.h>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
namespace mpi=boost::mpi;
#endif

#include "Gibbler.h"
#include "MpiDebug.h"

using namespace Moses;
using namespace std;

namespace Josiah {
  
  DeltaGain::DeltaGain(const GainFunctionHandle& gainFunction,const SampleVector& samples, size_t sampleId) :
      m_gainFunction(gainFunction),
      m_samples(samples),
      m_sampleId(sampleId) {}
  
  float DeltaGain::operator()(const TDeltaHandle& delta) {
    if (m_translations.size() == 0) {
      for (size_t i = 0; i < m_samples.size(); ++i) {
        Translation translation;
        if (i == m_sampleId) {
          delta->getNewSentence(translation);
        } else {
          const vector<Word>& targetWords = m_samples[i]->GetTargetWords();
          translation.reserve(targetWords.size());
          for (size_t j = 0; j < targetWords.size(); ++j) {
            translation.push_back(targetWords[j].GetFactor(0));
          }
        }
        m_translations.push_back(translation);
      }
    } else {
      m_translations[m_sampleId].clear();
      delta->getNewSentence(m_translations[m_sampleId]);
    }
    return m_gainFunction->Evaluate(m_translations);
  }
  
  SampleRankSelector::SampleRankSelector(
      const GainFunctionHandle& gainFunction,
      const OnlineLearnerHandle& onlineLearner,
      const TargetAssignerHandle& assigner,
      const WeightCollectorHandle& weightCollector) :
      m_gainFunction(gainFunction),
      m_onlineLearner(onlineLearner),
      m_assigner(assigner),
      m_weightCollector(weightCollector),
      m_burnin(false),
      m_ignoreUnknownWordPenalty(false),
      m_tolerance(0.0),
      m_alwaysUpdate(false),
      m_updateTarget(false)
  {
    m_unknownWordPenaltyName = StaticData::Instance().GetTranslationSystem
      (TranslationSystem::DEFAULT).GetUnknownWordPenaltyProducer()
          ->GetScoreProducerDescription();
  }
      
  void SampleRankSelector::SetSamples(const SampleVector& samples) {
        m_samples = samples;
        m_optimalGain.clear();
        m_optimalGain.resize(samples.size());
        m_optimalGainSolutionScores.clear();
        m_optimalGainSolutionScores.resize(samples.size());
  }

  void SampleRankSelector::SetIgnoreUnknownWordPenalty(bool ignore) {
    m_ignoreUnknownWordPenalty = ignore;
  }

  void WeightCollector::SetL1Normalise(bool l1normalise) {
    m_l1normalise = l1normalise;
  }
  void SampleRankSelector::SetAlwaysUpdate(bool alwaysUpdate) {
    m_alwaysUpdate = alwaysUpdate;
  }

  void SampleRankSelector::SetUpdateTarget(bool updateTarget) {
    m_updateTarget = updateTarget;
  }

  
  TDeltaHandle SampleRankSelector::Select(
                              size_t sampleId,
                              const TDeltaVector& deltas,
                              const TDeltaHandle& noChangeDelta, 
                              size_t iteration) 
  {

    //choose by sampling.
    if (m_burnin) {
       //DeltaGain gain(m_gainFunction,m_samples,sampleId);
       TDeltaHandle chosenDelta = m_burninSamplingSelector.Select(sampleId,deltas,noChangeDelta,iteration);
       //cerr << "BURN " << gain(chosenDelta) << endl;
       return chosenDelta;
    }
    TDeltaHandle chosenDelta = m_samplingSelector.Select(sampleId,deltas,noChangeDelta,iteration);
    DeltaGain gain(m_gainFunction,m_samples,sampleId);

  
    float chosenGain = gain(chosenDelta);
    float chosenScore = chosenDelta->getScore();

    //oracle
    int target = m_assigner->getTarget(deltas, chosenDelta, gain);
  
    if (target == -1)  return chosenDelta;
  
    //Only update if necessary, because it can be expensive
    if (m_onlineLearner->usesOptimalSolution()) {
        UpdateGainOptimalSol(deltas, noChangeDelta, sampleId, gain, target);
    }
  
    float targetScore = deltas[target]->getScore();
    float targetGain = gain(deltas[target]);
//    cerr << "CS " << chosenScore << " TS " << targetScore <<
//      " CG " << chosenGain << " TG " << targetGain << endl;
    //FVector oldWeights = WeightManager::instance().get();
    if (m_alwaysUpdate ||
         (chosenScore > targetScore && chosenGain+m_tolerance < targetGain)  ||
         (chosenScore < targetScore && chosenGain-m_tolerance > targetGain) ) {

      FVector chosenScores = chosenDelta->getSample().GetFeatureValues() - noChangeDelta->getScores() + chosenDelta->getScores();
      FVector targetScores = chosenDelta->getSample().GetFeatureValues() - noChangeDelta->getScores() + deltas[target]->getScores();
      if (m_ignoreUnknownWordPenalty) {
        chosenScores[m_unknownWordPenaltyName] = 0;
        targetScores[m_unknownWordPenaltyName] = 0;
      }
      
      m_onlineLearner->doUpdate(chosenScores,
                                targetScores,
                                m_optimalGainSolutionScores[sampleId],
                                chosenGain,
                                targetGain,
                                m_optimalGain[sampleId],
                                WeightManager::instance().get());
    }

     
    //cerr << "WEIGHTS: " << WeightManager::instance().get() << endl;
    //cerr << "BLEU: " << chosenGain << endl;
    m_weightCollector->updateWeights();

    //cerr << "WDIFF " << (WeightManager::instance().get() - oldWeights).l1norm() <<endl;
  
    //For approx doc bleu
    const Hypothesis* h = chosenDelta->getSample().GetSampleHypothesis();
    vector<const Factor*> trans;
    h->GetTranslation(&trans, 0);
    m_gainFunction->AddSmoothingStats(sampleId, trans);
    
    if (m_updateTarget) {
      return deltas[target];
    } else {
      return chosenDelta;
    }
  }
  

  
  void SampleRankSelector::UpdateGainOptimalSol(
      const TDeltaVector& deltas, 
      const TDeltaHandle& noChangeDelta,
      size_t sampleId,
      DeltaGain& gain,
      int target)
  {
    if (m_assigner->m_name != "Best") {
      //need to find the best solution, since we were
      //not using the best neighbour target assigner
      BestNeighbourTgtAssigner tgtAssigner;
      target = tgtAssigner.getTarget(deltas, noChangeDelta, gain);
    }
    if (target == -1) return; 
    float chosenGain = gain(deltas[target]);
    if (chosenGain > m_optimalGain[sampleId]) {
      m_optimalGain[sampleId] = chosenGain;
      m_optimalGainSolutionScores[sampleId] = deltas[target]->getSample().GetFeatureValues();
      m_optimalGainSolutionScores[sampleId] += deltas[target]->getScores();
      m_optimalGainSolutionScores[sampleId] -= noChangeDelta->getScores();
      if (m_ignoreUnknownWordPenalty) {
        m_optimalGainSolutionScores[sampleId][m_unknownWordPenaltyName] = 0;
      }
      VERBOSE(1,"New optimal gain " << m_optimalGain[sampleId] << endl);
    }
  }
  
  void SampleRankSelector::SetTemperature(float temp) {
    m_samplingSelector.SetTemperature(temp);
  }

  void SampleRankSelector::SetBurninAnnealer(AnnealingSchedule* schedule) {
    m_burninSamplingSelector.SetAnnealingSchedule(schedule);
  }
  
  void SampleRankSelector::BeginBurnin()  {
    m_burnin = true;
  }
  
  void SampleRankSelector::EndBurnin() {
    m_burnin = false;
  } 

  void SampleRankSelector::SetTolerance(float tolerance) {
    m_tolerance = tolerance;
  }
  
  
  size_t BestNeighbourTgtAssigner::getTarget(const TDeltaVector& deltas, const TDeltaHandle& noChangeDelta,
                                             DeltaGain& gf) {
  //Only do best neighbour for the moment
    float bestGain = -1;
    int bestGainIndex = -1;
    for (TDeltaVector::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
      float gain = gf(*i);
      if (gain > bestGain) {
        bestGain = gain;
        bestGainIndex = i - deltas.begin();
      }
    }
    IFVERBOSE(2) {
      if (bestGainIndex > -1) {
        cerr << "best nbr has score " << deltas[bestGainIndex]->getScore() << " and gain " << bestGain << endl;
      //cerr << "No change has score " << noChangeDelta->getScore() << " and gain " << noChangeDelta->getGain() << endl;    
      }
    }
    return bestGainIndex;
  }

  size_t ClosestBestNeighbourTgtAssigner::getTarget(const TDeltaVector& deltas, const TDeltaHandle& chosenDelta,
      DeltaGain& gf) {
  //Only do best neighbour for the moment
    float minScoreDiff = 10e10;
    int closestBestNbr = -1;
    float chosenGain = gf(chosenDelta);;
    float chosenScore = chosenDelta->getScore(); 
    for (TDeltaVector::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
      if (gf(*i) > chosenGain ) {
        float scoreDiff = chosenScore -  (*i)->getScore();
        if (scoreDiff < minScoreDiff) {
          minScoreDiff = scoreDiff;
          closestBestNbr = i - deltas.begin();
        }
      }
    }
  
    return closestBestNbr;
  }

 
  size_t ChiangBestNeighbourTgtAssigner::getTarget
    (const TDeltaVector& deltas, const TDeltaHandle& noChangeDelta, DeltaGain& gf) {
    float bestGain = -1e10;
    int bestGainIndex = -1;
    for (TDeltaVector::const_iterator i = deltas.begin(); i != deltas.end(); ++i) {
      float gain = gf(*i) + (*i)->getScore();
      if (gain > bestGain) {
        bestGain = gain;
        bestGainIndex = i - deltas.begin();
      }
    }
    IFVERBOSE(2) {
      if (bestGainIndex > -1) {
        cerr << "best nbr has score " << deltas[bestGainIndex]->getScore() << " and gain " << bestGain << endl;
      }
    }
    return bestGainIndex;
  }


  static void DumpWeights(const string& weightDumpStem,
       FVector averagedWeights, size_t size, size_t rank) {
    static size_t epoch = 0;
#ifdef MPI_ENABLED
    MPI_VERBOSE(1, "Before averaging, this node's average weights: " << averagedWeights << endl);
    mpi::communicator world;
    FVector totalWeights;
    mpi::reduce(world, averagedWeights, totalWeights, FVectorPlus(),0);
#endif
    if (rank == 0) {
#ifdef MPI_ENABLED
      averagedWeights = totalWeights / size;
#endif
      MPI_VERBOSE(1, "After averaging, average weights: " << averagedWeights << endl);
      ostringstream filename;
      filename << weightDumpStem << "_" << epoch;
      VERBOSE(1, "Dumping weights for epoch " << epoch << " to " << filename.str() << endl);
      averagedWeights.save(filename.str());
    }
    ++epoch;
    
  }


  WeightCollector::WeightCollector(size_t frequency, bool byBatch,
       const std::string& weightDumpStem, size_t size, size_t rank):
    m_frequency(frequency),
    m_byBatch(byBatch),
    m_weightDumpStem(weightDumpStem),
    m_updates(0),m_allUpdates(0),m_batches(0), m_size(size), m_rank(rank),
      m_l1normalise(false),m_lag(1), m_dumpCurrent(false)

  {
    if (m_frequency == 0) m_weightDumpStem = "";
    if (weightDumpStem.size()) {
      if (byBatch) {
        cerr << "Weight dumping by batch, frequency = " << frequency << endl;
      } else {
        cerr << "Weight dumping by sample, frequency = " << frequency << endl;
      }
    } else {
      cerr << "No weight dumping " << endl;
    }
    m_unknownWordPenaltyName = StaticData::Instance().GetTranslationSystem(
      TranslationSystem::DEFAULT).GetUnknownWordPenaltyProducer()
          ->GetScoreProducerDescription();

  }

  void WeightCollector::updateWeights() {
    ++m_allUpdates;
    if (m_allUpdates % m_lag) return;
    ++m_updates;
    FVector weights = WeightManager::instance().get();
    VERBOSE(1,"CURR_WEIGHTS " << weights << endl);
    if (m_l1normalise) {
      float uwp  = weights[m_unknownWordPenaltyName];
     // if (m_ignoreUnknownWordPenalty) {
        weights[m_unknownWordPenaltyName] = 0;
     // }
      weights /= weights.l1norm();
     // if (m_ignoreUnknownWordPenalty) {
        weights[m_unknownWordPenaltyName] = uwp;
     // }
    }
    m_totalWeights += weights;
    IFVERBOSE(1) {
      VERBOSE(1,"AVE_WEIGHTS " << getAverageWeights() << endl);
    }
 //   cerr << "WT " <<  (WeightManager::instance().get())[FName("PhraseModel_1")] << " ";
   // cerr <<  getAverageWeights() << endl; 
    if (m_weightDumpStem.length() && !m_byBatch && m_updates % m_frequency == 0) {
      if (m_dumpCurrent) {
        DumpWeights(m_weightDumpStem, WeightManager::instance().get(), m_size, m_rank);
      } else {
        DumpWeights(m_weightDumpStem, getAverageWeights(), m_size, m_rank);
      }
    }
  }

  void WeightCollector::endBatch() {
     ++m_batches;
  //  cerr << "Batch " << m_batches << " rank " << m_rank << endl;
    if (m_weightDumpStem.length() && m_byBatch && m_batches % m_frequency == 0) {
      DumpWeights(m_weightDumpStem, getAverageWeights(), m_size, m_rank);
    }
   }

  FVector WeightCollector::getAverageWeights() {
    assert(m_updates);
    return m_totalWeights/m_updates;
  }

  size_t WeightCollector::getBatchCount() {
    return m_batches;
  }

  void WeightCollector::SetLag(size_t lag) {
    if (lag == 0) lag = 1;
    m_lag = lag;
  }

  void WeightCollector::SetDumpCurrent(bool dumpCurrent) {
    m_dumpCurrent = dumpCurrent;
  }

}
