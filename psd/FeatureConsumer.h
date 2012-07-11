#ifndef moses_FeatureConsumer_h
#define moses_FeatureConsumer_h

#include "vw.h"
#include "ezexample.h"
#include <iostream>

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

