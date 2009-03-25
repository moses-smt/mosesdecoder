#pragma once

#include <vector>
#include <utility>
#include <ext/hash_map>
#include <map>

#include "Derivation.h"
#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"

namespace Moses { 
  class Factor;
  class SampleCollector;
}

namespace __gnu_cxx {
  template<> struct hash<std::vector<const Moses::Factor*> > {
    inline size_t operator()(const std::vector<const Moses::Factor*>& p) const {
      static const int primes[] = {8933, 8941, 8951, 8963, 8969, 8971, 8999, 9001, 9007, 9011};
      size_t h = 0;
      for (unsigned i = 0; i < p.size(); ++i)
        h += reinterpret_cast<size_t>(p[i]) * primes[i % 10];
      return h;
    }
  };
}

namespace Josiah {
  
 typedef std::vector<const Moses::Factor*> Translation;
 ostream& operator<<(ostream& out, const Translation& ws);
  
  /**
   * Collector that looks for a max (eg translation, derivation).
   **/
template <class M>
    class MaxCollector : public virtual Moses::SampleCollector {
    public:
      /** Should be called to report that an example of M was found in the sample*/
      void collectSample(const M&);
      /**argmax and max*/
      pair<const M*,float> getMax() const;
      /** n-best list*/
      void getNbest(vector<pair<const M*, float> >& nbest, size_t n) const;
      /**Estimate of the probability distribution */
      void getDistribution(map<const M*,float>& p) const;
      /** The sample at a given index.*/
      const M* getSample(size_t index) const;
      float getEntropy() const;
      
      virtual ~MaxCollector<M>(){}
   
    private:
      //maps the sample to the indices at which it was found.
      std::multimap<M,size_t> m_samples;
      //maps indices to samples
      vector<const M*> m_sampleList;
  
};

string ToString(const Translation& ws); 

class GibblerMaxTransDecoder : public virtual MaxCollector<Translation> {
 public:
  GibblerMaxTransDecoder();
  virtual void collect(Sample& sample);
  virtual  void Max(Translation&, size_t& count);
  /** Output the max translation whenever it changes */
  void setOutputMaxChange(bool outputMaxChange){m_outputMaxChange = outputMaxChange;}
  virtual void getCounts(std::vector<size_t>& counts) const;
  
  virtual ~GibblerMaxTransDecoder(){}

 private:
  int n;
  __gnu_cxx::hash_map<std::vector<const Factor*>, int> samples;
  int sent_num;
  
  bool m_outputMaxChange;
  Translation m_maxTranslation;
};

/**
 * Stop strategy which uses the count of the max (trans or deriv) to determine 
 * when to stop.
 **/
 template<class M>
     class MaxCountStopStrategy : public virtual Moses::StopStrategy {
       public:
         MaxCountStopStrategy(size_t minIterations, size_t maxIterations, size_t maxCount,  MaxCollector<M>* maxCollector)
         : m_minIterations(minIterations), m_maxIterations(maxIterations),m_maxCount(maxCount), m_maxCollector(maxCollector) {}
         virtual bool ShouldStop(size_t iterations) {
           if (iterations < m_minIterations) return false;
           if (iterations >= m_maxIterations) return true;
           pair<const M*, float> max = m_maxCollector->getMax();
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

