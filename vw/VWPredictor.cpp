#include "Classifier.h"
#include "vw.h"
#include "Util.h"
#include "ezexample.h"
#include <stdexcept>
#include <exception>
#include <string>

namespace Discriminative {

using namespace std;

VWPredictor::VWPredictor(const string &modelFile, const string &vwOptions)
{
  m_shared = true;
  m_VWInstance = VW::initialize(vwOptions + " -i " + modelFile);
  m_sharedVwInstance = false;
  m_ex = new ::ezexample(m_VWInstance, false);
  m_isFirstSource = m_isFirstTarget = true;
}

VWPredictor::VWPredictor(vw * instance, int index)
{
  m_VWInstance = instance;
  m_sharedVwInstance = true;
  m_ex = new ::ezexample(m_VWInstance, false);
  m_shared = true;
  m_index = index;
  m_isFirstSource = m_isFirstTarget = true;
}

void VWPredictor::AddLabelIndependentFeature(const StringPiece &name, float value)
{
  if (m_isFirstSource) {
    m_isFirstSource = false;
    m_ex->clear_features(); // removes all namespaces along with features
    m_ex->addns('s');
  }
  AddFeature(name, value);
}

void VWPredictor::AddLabelDependentFeature(const StringPiece &name, float value)
{
  if (m_isFirstTarget) {
    m_isFirstTarget = false;
    m_ex->addns('t');
  }
  AddFeature(name, value);
}

void VWPredictor::Train(const StringPiece &label, float loss)
{
  throw logic_error("Trying to train during prediction!");
}

float VWPredictor::Predict(const StringPiece &label)
{
  m_ex->set_label(label.as_string());
  m_isFirstSource = true;
  m_isFirstTarget = true;
  float loss = m_ex->predict();
  m_ex->remns(); // remove target namespace
  return loss;
}

void VWPredictor::AddFeature(const StringPiece &name, float value)
{
  m_ex->addf(name.as_string(), value);
}

void VWPredictor::Finish()
{
  if (m_sharedVwInstance)
    m_VWInstance = NULL;
  else
    VW::finish(*m_VWInstance);
}

VWPredictor::~VWPredictor()
{
  delete m_ex;
  if (!m_sharedVwInstance)
    VW::finish(*m_VWInstance);
}

} // namespace Discriminative
