#pragma once

#include <algorithm>
#include <vector>
#include <utility>
#include <map>

#include "ScoreComponentCollection.h"
#include "Phrase.h"
#include "SampleCollector.h"
#include "StopStrategy.h"


namespace Moses { 
  class Factor;
}

using namespace Moses;

namespace Josiah {
  
 typedef std::vector<const Moses::Factor*> Translation;
  std::ostream& operator<<(std::ostream& out, const Translation& ws);
  
  /**
   * Collector that looks for a max (eg translation, derivation).
   **/
template <class M>
    class MaxCollector : public virtual SampleCollector {
    public:
      MaxCollector<M>(const std::string& name) : m_name(name), m_outputMaxChange(false) {}
      /** Should be called to report that an example of M was found in the sample*/
      void collectSample(const M&);
      /**argmax and max*/
      virtual std::pair<const M*,float> getMax() const;
      /** n-best list. Set n=0 to get all translations*/
      void getNbest(std::vector<std::pair<const M*, float> >& nbest, size_t n) const;
      /**Estimate of the probability distribution */
      void getDistribution(std::map<const M*,double>& p) const;
      /**Print the probability distribution to a file*/
      void printDistribution(std::ostream& out) const;
      /** Output the max  whenever it changes */
      void setOutputMaxChange(bool outputMaxChange){m_outputMaxChange = outputMaxChange;}
      /** The sample at a given index.*/
      const M* getSample(size_t index) const;
      float getEntropy() const;
      void reset();
      
      virtual ~MaxCollector<M>(){}
   
    private:
      //maps the sample to the indices at which it was found.
      std::map<M,std::vector<size_t> > m_samples;
      //maps indices to samples
      std::vector<const M*> m_sampleList;
      //used for debug messages
      std::string m_name;
      //output when max changes?
      bool m_outputMaxChange;
      const M* m_max;
  
};

std::string ToString(const Translation& ws); 

class GibblerMaxTransDecoder : public virtual MaxCollector<Translation> {
 public:
  GibblerMaxTransDecoder() : MaxCollector<Translation>("Trans") {}
  virtual void collect(Sample& sample);
  /** Do mbr decoding */
  std::pair<const Translation*,float> getMbr(size_t mbrSize) const;
  virtual ~GibblerMaxTransDecoder(){}

 private:
};

/**
 * Stop strategy which uses the count of the max (trans or deriv) to determine 
 * when to stop.
 **/
 template<class M>
     class MaxCountStopStrategy : public virtual StopStrategy {
       public:
         MaxCountStopStrategy(size_t minIterations, size_t maxIterations, size_t maxCount,  MaxCollector<M>* maxCollector)
         : m_minIterations(minIterations), m_maxIterations(maxIterations),m_maxCount(maxCount), m_maxCollector(maxCollector) {}
         virtual bool ShouldStop(size_t iterations) {
           if (iterations < m_minIterations) return false;
           if (iterations >= m_maxIterations) return true;
           std::pair<const M*, float> max = m_maxCollector->getMax();
           size_t count = (size_t)(max.second * m_maxCollector->N() +0.5);
           return (count >= m_maxCount);
         }
         virtual ~MaxCountStopStrategy() {}
    
       private:
         size_t m_minIterations;
         size_t m_maxIterations;
         size_t m_maxCount;
         MaxCollector<M>* m_maxCollector;
     };


}

