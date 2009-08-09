#pragma once
#include <vector>


namespace Josiah {
  
  class Sample;
  
  /**
   * Used by the operators to collect samples, for example to count ngrams, or just to print
   * them out. 
   **/
  class SampleCollector {
  public:
    SampleCollector(): m_totalImportanceWeight(0), m_n(0)  {}
    virtual void addSample(Sample& sample, double importanceWeight);
    /** Number of samples */
    size_t N() const {return m_n;}
    virtual ~SampleCollector() {}
    void reset() {
      m_totalImportanceWeight = 0;
      m_n = 0;
      m_importanceWeights.clear();
      m_normalisedImportanceWeights.clear();
    }
  protected:
    /** The actual collection.*/
    virtual void collect(Sample& sample) = 0;
    /** The log of the total importance weight */
    float getTotalImportanceWeight() const {return m_totalImportanceWeight;}
    /** Normalised importance weights  - in probability space*/
    const std::vector<double>& getImportanceWeights() const;
    
    
  private:
    double m_totalImportanceWeight; //normalisation factor
    std::vector<double> m_importanceWeights; //unnormalised weights, in log space
    mutable std::vector<double> m_normalisedImportanceWeights; //normalised, in prob space
    size_t m_n;
  };
  
  class PrintSampleCollector  : public virtual SampleCollector {
  public:
    virtual void collect(Sample& sample);
    virtual ~PrintSampleCollector() {}
  };
  
  
}

