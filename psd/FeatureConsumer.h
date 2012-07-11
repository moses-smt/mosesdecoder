#ifndef moses_FeatureConsumer_h
#define moses_FeatureConsumer_h

#include <iostream>

// forward declarations to avoid dependency VW 
struct VW::vw;
class ezexample;

// abstract consumer
class FeatureConsumer
{
public:
  virtual void SetNamespace(const string &ns, bool shared) = 0;
  virtual void AddFeature(const string &name) = 0;
  virtual void AddFeature(const string &name, real value) = 0;
  virtual void Train(const string &label, float loss) = 0;
  virtual float Predict(const string &label) = 0;
  virtual void FinishExample() = 0;
  virtual void Finish() = 0;
};

// consumer that builds VW training files
class VWFileTrainConsumer : public FeatureConsumer
{
public:
  virtual void SetNamespace(const string &ns, bool shared);
  //newline and prepend share
  virtual void AddFeature(const string &name);
  //print " "
  virtual void AddFeature(const string &name, real value);
  //look in file
  virtual void FinishExample();
  //nweline (look in file)
  virtual void Finish();
  //close the file
  virtual void Train(const string &label, float loss);
  virtual float Predict(const string &label);

  VWFileBasedConsumer(const string &outputFile);

private:
  std::ostream os;
};

// abstract consumer that trains/predicts using VW library interface
class VWLibraryConsumer : public FeatureConsumer
{
public:
  virtual void SetNamespace(const string &ns, bool shared);
  virtual void AddFeature(const string &name);
  virtual void AddFeature(const string &name, real value);
  virtual void FinishExample();
  virtual void Finish();

private:
  VW::vw *m_VWInstance;
  ezexample *m_ex;
}

// train using VW
class VWLibraryTrainConsumer : public VWLibraryConsumer
{
public:
  VWLibraryTrainConsumer(const string &modelFile);
  virtual void Train(const string &label, float loss);
  virtual float Predict(const string &label);
};

// predict using VW
class VWLibraryPredictConsumer : public VWLibraryConsumer
{
public:
  VWLibraryPredictConsumer(const string &modelFile);
  virtual void Train(const string &label, float loss);
  virtual float Predict(const string &label);
};

