#ifndef moses_FeatureConsumer_h
#define moses_FeatureConsumer_h


class FeatureConsumer
{
public:
	virtual void SetNamespace(const string &ns, bool shared) = 0;
	virtual void AddFeature(const string &name) = 0;
	virtual void AddFeature(const string &name, real value) = 0;
  virtual void Train(const string &label, float loss) = 0;
  virtual float Predict(const string &label) = 0;
  virtual void Finish();
};

class VWFileTrainConsumer : public FeatureConsumer
{
public:
  VWFileBasedConsumer(const string &outputFile);
};

class VWLibraryConsumer : public FeatureConsumer
{                          
public:
	virtual void SetNamespace(const string &ns, bool shared) = 0;
	virtual void AddFeature(const string &name) = 0;
	virtual void AddFeature(const string &name, real value) = 0;
  virtual void Finish();
private:
  VW *m_VWInstance;
}

class VWLibraryPredictConsumer : public VWLibraryConsumer
{
public:
  VWLibraryPredictConsumer() { m_VWInstance = NULL; }
  LoadModel(const string &model);

};

class VWLibraryTrainConsumer : public VWLibraryConsumer
{
public:
  VWLibraryTrainConsumer() { m_VWInstance = NULL; }
  virtual void Finish() { VWLibraryConsumer::finish(); modelHdl.close(); }
  bool InitializeOutput(const string &model);

};

