#ifndef moses_FeatureConsumer_h
#define moses_FeatureConsumer_h

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <deque>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/iostreams/filtering_stream.hpp> 
#include <boost/iostreams/filter/gzip.hpp> 

// #ifdef HAVE_VW
  // forward declarations to avoid dependency on VW 
  struct vw;
  class ezexample;
// #endif

namespace PSD
{

// abstract consumer
class FeatureConsumer
{
public:
  virtual void SetNamespace(char ns, bool shared) = 0;
  virtual void AddFeature(const std::string &name) = 0;
  virtual void AddFeature(const std::string &name, float value) = 0;
  virtual void Train(const std::string &label, float loss) = 0;
  virtual float Predict(const std::string &label) = 0;
  virtual void FinishExample() = 0;
  virtual void Finish() = 0;
};

// consumer that builds VW training files
class VWFileTrainConsumer : public FeatureConsumer
{
public:
  VWFileTrainConsumer(const std::string &outputFile);

  // FeatureConsumer interface implementation
  virtual void SetNamespace(char ns, bool shared);
  virtual void AddFeature(const std::string &name);
  virtual void AddFeature(const std::string &name, float value);
  virtual void FinishExample();
  virtual void Finish();
  virtual void Train(const std::string &label, float loss);
  virtual float Predict(const std::string &label);

private:
  boost::iostreams::filtering_ostream m_bfos;
  std::deque<std::string> m_outputBuffer;

  void WriteBuffer();
  std::string EscapeSpecialChars(const std::string &str);
};

// #ifdef HAVE_VW
  // abstract consumer that trains/predicts using VW library interface
  class VWLibraryConsumer : public FeatureConsumer, private boost::noncopyable
  {
  public:
    virtual void SetNamespace(char ns, bool shared);
    virtual void AddFeature(const std::string &name);
    virtual void AddFeature(const std::string &name, float value);
    virtual void FinishExample();
    virtual void Finish();
  
  protected:
    ::vw *m_VWInstance;
    ::ezexample *m_ex;
    // this contains state about which namespaces are shared
    bool m_shared;
    // if true, then the VW instance is owned by an external party and should NOT be
    // deleted at end; if false, then we own the VW instance and must clean up after it.
    bool m_sharedVwInstance;
    int m_index;

    ~VWLibraryConsumer();
  };
  
  // train using VW
  class VWLibraryTrainConsumer : public VWLibraryConsumer
  {
  public:
    VWLibraryTrainConsumer(const std::string &modelFile);
    virtual void Train(const std::string &label, float loss);
    virtual float Predict(const std::string &label);
    virtual void FinishExample();
  };
  
  // predict using VW
  class VWLibraryPredictConsumer : public VWLibraryConsumer
  {
  public:
    VWLibraryPredictConsumer(const std::string &modelFile);
    virtual void Train(const std::string &label, float loss);
    virtual float Predict(const std::string &label);

    friend class VWLibraryPredictConsumerFactory;

  private:
    VWLibraryPredictConsumer(vw * instance, int index);
  };

  // object pool of VWLibraryPredictConsumers
  class VWLibraryPredictConsumerFactory : private boost::noncopyable
  {
  public:
    VWLibraryPredictConsumerFactory(const std::string &modelFile, const int poolSize);

    VWLibraryPredictConsumer * Acquire();
    void Release(VWLibraryPredictConsumer * fc);

    ~VWLibraryPredictConsumerFactory();

  private:
    ::vw *m_VWInstance;
    int m_firstFree;
    std::vector<int> m_nextFree;
    std::vector<VWLibraryPredictConsumer *> m_consumers;
    boost::mutex m_mutex;
    boost::condition_variable m_cond;
  };

// #endif // HAVE_VW

} // namespace PSD

#endif // moses_FeatureConsumer_h
