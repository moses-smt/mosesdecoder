#include "vw.h"
#include "ezexample.h"
#include "FeatureConsumer.h"

VWLibraryTrainConsumer::VWLibraryConsumer()
{
    m_vwInstance = NULL;
    m_ex = NULL;
}

VWLibraryTrainConsumer::VWLibraryConsumer(const string &modelFile)
{
    m_vwInstance = new VW::initialize("--hash all -q st --noconstant -f " + modelFile);
    m_ex = new ezexample(m_vwInstance,false);
}

~VWLibraryTrainConsumer::VWLibraryConsumer();
{
    delete m_vwInstance;
    delete m_ex;
}

VWLibraryPredictConsumer::VWLibraryConsumer(const string &modelFile)
{
    m_vwInstance = new VW::initialize("--hash all -q st --noconstant -i " + modelFile);
    m_ex = new ezexample(m_vwInstance,false);
}

~VWLibraryConsumer::VWLibraryConsumer()
{
    delete m_vwInstance;
    delete m_ex;
}

void VWLibraryConsumer::SetNamespace(const string &ns, bool shared)
{
    if(!m_shared)
    {
        --(*m_ex);
    }
    (*m_ex)(vw_namespace(ns));
    m_shared = shared;
}

void VWLibraryConsumer::FinishExample()
{
    m_ex.clear();
}

void VWLibraryConsumer::Finish()
{
    finish(m_VWInstance);
}

void VWLibraryConsumer::AddFeature(const string &name)
{
    m_ex(name);
}

void VWLibraryConsumer::AddFeature(const string &name, real value)
{
    (*m_ex)(name,value);
}

//Note : model passed to initialize
/*float VWLibraryPredictConsumer::LoadModel(const string &model)
{

}*/

float VWLibraryPredictConsumer::Predict(const string &label)
{
    return m_ex->predict();
}

VWLibraryTrainConsumer::Train(const string &label, float loss)
{
    m_ex->predict(loss);
}
