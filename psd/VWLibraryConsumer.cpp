#include "vw.h"
#include "ezexample.h"
#include "FeatureConsumer.h"
#include <stdexcept>
#include <exception>

using namespace std;

// 
// VWLibraryConsumer
//

void VWLibraryConsumer::SetNamespace(const string &ns, bool shared)
{
  if (!m_shared)
    m_ex->remns();

  m_ex->addns(vw_namespace(ns).namespace_letter);
  m_shared = shared;
}

void VWLibraryConsumer::AddFeature(const string &name)
{
  m_ex->addf(name);
}

void VWLibraryConsumer::AddFeature(const string &name, real value)
{
  m_ex->addf(name, value);
}

void VWLibraryConsumer::FinishExample()
{
  m_ex->clear();
}

void VWLibraryConsumer::Finish()
{
  finish(m_VWInstance);
}

~VWLibraryConsumer::VWLibraryConsumer()
{
  delete m_vwInstance;
  delete m_ex;
}

//
// VWLibraryTrainConsumer
//

VWLibraryTrainConsumer::VWLibraryConsumer(const string &modelFile)
{
  m_vwInstance = new VW::initialize("--hash all -q st --noconstant -f " + modelFile);
  m_ex = new ezexample(m_vwInstance, false);
}

void VWLibraryTrainConsumer::Train(const string &label, float loss)
{
  m_ex->set_label(label, loss);
  m_ex->train();
}

void VWLibraryTrainConsumer::FinishExample()
{
  m_ex->finish();
  VWLibraryConsumer::FinishExample();
}

float VWLibraryTrainConsumer::Predict(const string &label)
{
  throw logic_error("Trying to predict during training!");
}

//
// VWLibraryPredictConsumer
//

VWLibraryPredictConsumer::VWLibraryConsumer(const string &modelFile)
{
  m_vwInstance = new VW::initialize("--hash all -q st --noconstant -i " + modelFile);
  m_ex = new ezexample(m_vwInstance, false);
}

void VWLibraryTrainConsumer::Train(const string &label, float loss)
{
  throw logic_error("Trying to train during prediction!");
}

float VWLibraryPredictConsumer::Predict(const string &label)
{
  m_ex->set_label(label);
  return m_ex->predict();
}

