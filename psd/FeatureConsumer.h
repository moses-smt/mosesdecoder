#ifndef moses_FeatureConsumer_h
#define moses_FeatureConsumer_h

#include <iostream>

struct VW::vw;
struct ezexample;

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
    void SetNamespace(const string &ns, bool shared) = 0;
    //newline and prepend share
	void AddFeature(const string &name) = 0;
	//print " "
	void AddFeature(const string &name, real value) = 0;
	//look in file
	void FinishExample() = 0;
	//nweline (look in file)
    void Finish();
    //close the file

    public:
    VWFileBasedConsumer(const string &outputFile);

    private:
    ostream os;
};

class VWLibraryConsumer : public FeatureConsumer
{
    public:
    VWLibraryConsumer();
	void SetNamespace(const string &ns, bool shared) = 0;
	void AddFeature(const string &name) = 0;
	void AddFeature(const string &name, real value) = 0;
	void FinishExample() = 0;
    void Finish();

    private:
    VW::vw *m_VWInstance;
    ezexample *m_ex;
}

class VWLibraryPredictConsumer : public VWLibraryConsumer
{
    public:
    VWLibraryPredictConsumer(const string &modelFile);
    ~VWLibraryPredictConsumer();
    float Predict(const string &label);
};

class VWLibraryTrainConsumer : public VWLibraryConsumer
{
    public:
    VWLibraryTrainConsumer(const string &modelFile);
    ~VWLibraryTrainConsumer();
    void Train();
};

