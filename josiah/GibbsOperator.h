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

#include "DummyScoreProducers.h"
#include "Gibbler.h"
#include "Hypothesis.h"
#include "TranslationDelta.h"
#include "TranslationOptionCollection.h"
#include "WordsRange.h"

namespace Moses {

  class Sample;
  class SampleCollector;
  class TranslationDelta;
  
  typedef boost::mt19937 base_generator_type;
  
  /**
    * Wraps the random number generation and enables seeding.
    **/
  class RandomNumberGenerator {
    //mersenne twister - and why not?
    
    
    public:
      RandomNumberGenerator();
      double next() {return m_random();}
      void setSeed(uint32_t seed){
          m_generator.seed(seed);
          cerr << "Setting random seed to " << seed << endl;
      }
      
    private:
      boost::uniform_real<> m_dist; 
      base_generator_type m_generator;
      boost::variate_generator<base_generator_type&, boost::uniform_real<> > m_random;
      
  };

  /** Abstract base class for gibbs operators **/
  class GibbsOperator {
    public:
      GibbsOperator(const string& name) : m_name(name), T(1) {}
        /**
          * Run an iteration of the Gibbs sampler, updating the hypothesis.
          **/
        virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc) = 0;
        const string& name() const {return m_name;}
        virtual ~GibbsOperator() {}
        
        static void setRandomSeed(uint32_t seed) {m_random.setSeed(seed);}
				void SetAnnealingTemperature(const double t) { assert(t >= 1.0); T = t; }
				void Quench() { T = 1.0; }
        
     protected:
        /**
          * Randomly select and apply one of the translation deltas.
          **/
        void doSample(std::vector<TranslationDelta*>& deltas, TranslationDelta* noChangeDelta);
        
        string m_name;
				double T;  // annealing temperature
        
    private:
      static RandomNumberGenerator m_random;
      
  };
  
  /**
    * Operator that keeps ordering constant, but visits each (internal) source word boundary, and 
    * merge or split the segment(s) at that boundary, and update the translation.
    **/
  class MergeSplitOperator : public virtual GibbsOperator {
    public:
        MergeSplitOperator() : GibbsOperator("merge-split") {}
        virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc);
        virtual ~MergeSplitOperator() {}
    
    private:
        
  
  };
  
  /**
    * Operator which may update any translation option, but may not change segmentation or ordering.
    **/
  class TranslationSwapOperator : public virtual GibbsOperator {
    public:
      TranslationSwapOperator() : GibbsOperator("translation-swap") {}
      virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc);
      virtual ~TranslationSwapOperator() {}
  };
  
  /**
   * Operator which performs local reordering provided both source segments and target segments are contiguous, and that the swaps
   * will not violate the reordering constraints of the model
   **/
  class FlipOperator : public virtual GibbsOperator {
  public:
    FlipOperator() : GibbsOperator("flip") {}
    virtual void doIteration(Sample& sample, const TranslationOptionCollection& toc);
    virtual const string& name() const {return m_name;}
    virtual ~FlipOperator() {}
    
  private:
    string m_name;
    //bool CheckValidReordering(const Hypothesis* leftTgtHypo, const Hypothesis *rightTgtHypo, const Hypothesis* leftPrevHypo, const Hypothesis* rightNextHypo, float & totalDistortion);
    bool CheckValidReordering(const Hypothesis* leftTgtHypo, const Hypothesis *rightTgtHypo, const Hypothesis* leftTgtPrevHypo, const Hypothesis* leftTgtNextHypo, const Hypothesis* rightTgtPrevHypo, const Hypothesis* rightTgtNextHypo, float & totalDistortion);
    void CollectAllSplitPoints(Sample& sample, vector<int> &splitPoints);
  };
 
}

