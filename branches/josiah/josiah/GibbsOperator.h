// vim:tabstop=2
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

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iomanip>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>


#include "TranslationDelta.h"
#include "GibbsOperatorIterator.h"
#include "TypeDef.h"

namespace Moses {
  class Hypothesis;
  class TranslationOptionCollection;
  class WordsRange;
}

using namespace Moses;

namespace Josiah {

  class Sample;
  class GainFunction;
  class Sampler;
  class SampleAcceptor;
  class TargetAssigner;
  class SampleCollector;
  class MHAcceptor;
    
  
  typedef boost::mt19937 base_generator_type;
  
  
  template<class T>
      T log_sum (T log_a, T log_b)
  {
    T v;
    if (log_a < log_b) {
      v = log_b+log ( 1 + exp ( log_a-log_b ));
    } else {
      v = log_a+log ( 1 + exp ( log_b-log_a ));
    }
    return ( v );
  }
  
  /**
    * Wraps the random number generation and enables seeding.
    **/
  class RandomNumberGenerator {
    //mersenne twister - and why not?
    
    
    public:
      static RandomNumberGenerator& instance() {return s_instance;}
      double next() {return m_random();}
      void setSeed(uint32_t seed){
          m_generator.seed(seed);
          std::cerr << "Setting random seed to " << seed << std::endl;
      }
    
      size_t getRandomIndexFromZeroToN(size_t n) {
        double increment = 1.0/n;
        double random = next();
        double val = increment;
        for (size_t i = 0; i < n; ++i, val+= increment) {
          if (random > val)
            continue;
          else 
            return i;
        }
      }
      
    private:
      static RandomNumberGenerator s_instance;
      RandomNumberGenerator();
      boost::uniform_real<> m_dist; 
      base_generator_type m_generator;
      boost::variate_generator<base_generator_type&, boost::uniform_real<> > m_random;
      
  };
  
  /** Abstract base class for gibbs operators **/
  class GibbsOperator {
    public:
        GibbsOperator(const std::string& name, float prob) : m_name(name), T(1), m_gf(NULL), m_useApproxDocBleu(false), m_OpIterator(NULL), m_prob(prob), m_acceptor(NULL), m_mhacceptor(NULL) {}
        /**
          * Run an iteration of the Gibbs sampler, updating the hypothesis.
          **/
        virtual ~GibbsOperator();
        virtual void scan(Sample& sample, const TranslationOptionCollection& toc) = 0;
        const std::string& name() const {return m_name;}
        void SetAnnealingTemperature(const double t);
        void Quench() ;
        void SetGainFunction(const GainFunction *gf) {m_gf = gf;}
        const GainFunction* GetGainFunction() {return m_gf;}
        int chooseTargetAssignment(const std::vector<TranslationDelta*>& deltas);
        void SetSampler(Sampler* sampler) {m_sampler = sampler;}
        Sampler* GetSampler()  {return m_sampler;}
        void disableGainFunction() { m_gf_bk = m_gf; m_gf = NULL; }
        void enableGainFunction() { m_gf = m_gf_bk; m_gf_bk = NULL; }
        void doOnlineLearning(std::vector<TranslationDelta*>& deltas, TranslationDelta* noChangeDelta, TranslationDelta* chosen);
        void addSampleAcceptor(SampleAcceptor* acceptor) { m_acceptor = acceptor;}
        void addMHAcceptor(MHAcceptor* acceptor) {m_mhacceptor = acceptor;}
        void addTargetAssigner(TargetAssigner* assigner) { m_assigner = assigner;}
        void UseApproxDocBleu(bool use) { m_useApproxDocBleu = use;}
        void UpdateGainOptimalSol(const std::vector<TranslationDelta*>& deltas, TranslationDelta* chosen, int target, TranslationDelta* noChangeDelta);
        bool keepGoing()  {return m_OpIterator->keepGoing();}
        void resetIterator() { m_OpIterator->reset();}
        void setGibbsLMInfo(const std::map <LanguageModel*, int> & lms) { m_LMInfo = lms;}
        std::map <LanguageModel*, int> & getGibbsLMInfo() {return m_LMInfo;}
        float GetScanProb() const {return m_prob;}
     protected:
        /**
          * Randomly select and apply one of the translation deltas.
          **/
        void doSample(std::vector<TranslationDelta*>& deltas, TranslationDelta* noChangeDelta, Sample& sample);
        
        std::string m_name;
				double T;  // annealing temperature
        GibbsOperatorIterator* m_OpIterator;  
        float m_prob; // the probability of sampling this operator
    private:
      static RandomNumberGenerator m_random;
      const GainFunction* m_gf;
      const GainFunction* m_gf_bk;
      SampleAcceptor* m_acceptor;
      MHAcceptor* m_mhacceptor;
      TargetAssigner* m_assigner;
      Sampler* m_sampler;
      bool m_useApproxDocBleu;
      std::map< LanguageModel*, int> m_LMInfo;
  };
  
  /**
    * Operator that keeps ordering constant, but visits each (internal) source word boundary, and 
    * merge or split the segment(s) at that boundary, and update the translation.
    **/
  class MergeSplitOperator : public virtual GibbsOperator {
    public:
      MergeSplitOperator(bool randomScan = false, float scanProb) : GibbsOperator("merge-split", scanProb) {
        if (randomScan)
          m_OpIterator = new MergeSplitRandomIterator();
        else
        m_OpIterator = new MergeSplitIterator();
      }
      virtual ~MergeSplitOperator() {}
      virtual void scan(Sample& sample, const TranslationOptionCollection& toc);
  };
  
  /**
    * Operator which may update any translation option, but may not change segmentation or ordering.
    **/
  class TranslationSwapOperator : public virtual GibbsOperator {
    public:
      TranslationSwapOperator(bool randomScan = false, float scanProb) :  GibbsOperator("translation-swap", scanProb) {
        if (randomScan)
          m_OpIterator = new SwapRandomIterator();
        else
          m_OpIterator = new SwapIterator();
      }
      virtual ~TranslationSwapOperator() {}
      virtual void scan(Sample& sample, const TranslationOptionCollection& toc);
  };
  
  /**
   * Operator which performs local reordering provided both source segments and target segments are contiguous, and that the swaps
   * will not violate the reordering constraints of the model
   **/
  class FlipOperator : public virtual GibbsOperator {
  public:
    FlipOperator(bool randomScan = false, float scanProb) : GibbsOperator("flip", scanProb) {
      if (randomScan)
        m_OpIterator = new FlipRandomIterator(this);
      else
        m_OpIterator = new FlipIterator(this);
    }
    virtual ~FlipOperator() {}
    virtual void scan(Sample& sample, const TranslationOptionCollection& toc);
    
    const vector<size_t> & GetSplitPoints() {
      return m_splitPoints;
    }
  private:
    bool CheckValidReordering(const Hypothesis* leftTgtHypo, const Hypothesis *rightTgtHypo, const Hypothesis* leftTgtPrevHypo, const Hypothesis* leftTgtNextHypo, const Hypothesis* rightTgtPrevHypo, const Hypothesis* rightTgtNextHypo, float & totalDistortion);
    void CollectAllSplitPoints(Sample& sample);
    vector<size_t> m_splitPoints;
  };
  
  
}

