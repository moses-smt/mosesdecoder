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
    SampleCollector():  m_n(0)  {}
    virtual void addSample(Sample& sample);
    /** Number of samples */
    size_t N() const {return m_n;}
    virtual ~SampleCollector() {}
    void reset() {
      m_n = 0;
    }
    void SetN(size_t n) { m_n = n;}
  protected:
    /** The actual collection.*/
    virtual void collect(Sample& sample) = 0;
    
    
  private:
    size_t m_n;
  };
  
  class PrintSampleCollector  : public virtual SampleCollector {
  public:
    virtual void collect(Sample& sample);
    virtual ~PrintSampleCollector() {}
  };
  
  
}

