#include "FeatureConsumer.h"
#include "vw.h"
#include "Util.h"
#include "ezexample.h"
#include <stdexcept>
#include <exception>
#include <string>

using namespace std;

namespace PSD
{

// 
// VWLibraryConsumer
//

void VWLibraryConsumer::SetNamespace(char ns, bool shared)
{
  if (!m_shared)
    m_ex->remns();

  m_ex->addns(ns);
  m_shared = shared;
}

void VWLibraryConsumer::AddFeature(const string &name)
{
  m_ex->addf(name);
}

void VWLibraryConsumer::AddFeature(const string &name, float value)
{
  m_ex->addf(name, value);
}

void VWLibraryConsumer::FinishExample()
{
  m_ex->clear_features();
}

void VWLibraryConsumer::Finish()
{
  VW::finish(*m_VWInstance);
}

VWLibraryConsumer::~VWLibraryConsumer()
{
  delete m_VWInstance;
  delete m_ex;
}

//
// VWLibraryTrainConsumer
//

VWLibraryTrainConsumer::VWLibraryTrainConsumer(const string &modelFile)
{
  m_VWInstance = new ::vw(VW::initialize("--hash all -q st --noconstant -f " + modelFile));
  m_ex = new ::ezexample(m_VWInstance, false);
}

void VWLibraryTrainConsumer::Train(const string &label, float loss)
{
  m_ex->set_label(label + Moses::SPrint(loss));
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

VWLibraryPredictConsumer::VWLibraryPredictConsumer(const string &modelFile)
{
  m_VWInstance = new ::vw(VW::initialize("--hash all -q st --noconstant -i " + modelFile));
  m_ex = new ::ezexample(m_VWInstance, false);
}

void VWLibraryPredictConsumer::Train(const string &label, float loss)
{
  throw logic_error("Trying to train during prediction!");
}

float VWLibraryPredictConsumer::Predict(const string &label)
{
  m_ex->set_label(label);
  return m_ex->predict();
}

} // namespace PSD
