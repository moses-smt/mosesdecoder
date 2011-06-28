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

#pragma once

/**
 * Strategies used to select samples proposed by the gibbs operators.
**/

#include <vector>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include "AnnealingSchedule.h"
#include "Gibbler.h"
#include "TranslationDelta.h"

namespace Josiah {

  /**
    * Abstract base class for sample selection strategy.
   **/
  class DeltaSelector {
    public:
      virtual TDeltaHandle Select(     size_t sampleId,  //which sample is selected from the batch
                                       const TDeltaVector& deltas,
                                       const TDeltaHandle& noChangeDelta, 
                                       size_t iteration) = 0;
      virtual void BeginBurnin() {}
      virtual void EndBurnin() {}
      virtual void SetSamples(const SampleVector& samples) {}
      virtual ~DeltaSelector() {}
  };
  
  
  /**
    * Selector that samples the delta by converting the scores to probabilities.
   **/
  class SamplingSelector : public DeltaSelector {
    public:
      SamplingSelector();
      virtual TDeltaHandle  Select(    size_t sampleId,
                                       const TDeltaVector& deltas,
                                       const TDeltaHandle& noChangeDelta, 
                                       size_t iteration);
      void SetAnnealingSchedule(AnnealingSchedule* annealingSchedule);
      void SetTemperature(float temperature);
    
    private:
      //Note that the annealingSchedule overrides the temperature
      AnnealingSchedule* m_annealingSchedule;
      float m_temperature;
  };
  
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
        return (size_t)(next()*n);
      }
      
    private:
      static RandomNumberGenerator s_instance;
      RandomNumberGenerator();
      boost::uniform_real<> m_dist; 
      base_generator_type m_generator;
      boost::variate_generator<base_generator_type&, boost::uniform_real<> > m_random;
      
  };
  
  struct RandomIndex {
    ptrdiff_t operator() (ptrdiff_t max) {
      
      return static_cast<ptrdiff_t>(RandomNumberGenerator::instance().getRandomIndexFromZeroToN(max));
    }
  };


};
