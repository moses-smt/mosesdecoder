#ifndef moses_FeatureConsumer_h
#define moses_FeatureConsumer_h

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <deque>

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

  // interface implementation
  virtual void SetNamespace(char ns, bool shared);
  virtual void AddFeature(const std::string &name);
  virtual void AddFeature(const std::string &name, float value);
  virtual void FinishExample();
  virtual void Finish();
  virtual void Train(const std::string &label, float loss);
  virtual float Predict(const std::string &label);

private:
  std::ofstream m_os;
  std::deque<std::string> m_outputBuffer;

  void WriteBuffer();
  std::string EscapeSpecialChars(const std::string &str);
};

// #ifdef HAVE_VW
  // abstract consumer that trains/predicts using VW library interface
  class VWLibraryConsumer : public FeatureConsumer
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
    bool m_shared;

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
  };
// #endif // HAVE_VW

} // namespace PSD

#endif // moses_FeatureConsumer_h
