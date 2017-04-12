#include <iostream>

#include "Classifier.h"
#include "vw.h"
#include "ezexample.h"
#include "../moses/Util.h"

namespace Discriminative
{

using namespace std;

VWPredictor::VWPredictor(const string &modelFile, const string &vwOptions)
{
  m_VWInstance = VW::initialize(VW_DEFAULT_OPTIONS + " -i " + modelFile + vwOptions);
  m_VWParser = VW::initialize(VW_DEFAULT_PARSER_OPTIONS + vwOptions + " --noop");
  m_sharedVwInstance = false;
  m_ex = new ::ezexample(m_VWInstance, false, m_VWParser);
  m_isFirstSource = m_isFirstTarget = true;
}

VWPredictor::VWPredictor(vw *instance, const string &vwOptions)
{
  m_VWInstance = instance;
  m_VWParser = VW::initialize(vwOptions + " --noop");
  m_sharedVwInstance = true;
  m_ex = new ::ezexample(m_VWInstance, false, m_VWParser);
  m_isFirstSource = m_isFirstTarget = true;
}

VWPredictor::~VWPredictor()
{
  delete m_ex;
  VW::finish(*m_VWParser);
  if (!m_sharedVwInstance)
    VW::finish(*m_VWInstance);
}

FeatureType VWPredictor::AddLabelIndependentFeature(const StringPiece &name, float value)
{
  // label-independent features are kept in a different feature namespace ('s' = source)

  if (m_isFirstSource) {
    // the first feature of a new example => create the source namespace for
    // label-independent features to live in
    m_isFirstSource = false;
    m_ex->finish();
    m_ex->addns('s');
    if (DEBUG) std::cerr << "VW :: Setting source namespace\n";
  }
  return AddFeature(name, value); // namespace 's' is set up, add the feature
}

FeatureType VWPredictor::AddLabelDependentFeature(const StringPiece &name, float value)
{
  // VW does not use the label directly, instead, we do a Cartesian product between source and target feature
  // namespaces, where the source namespace ('s') contains label-independent features and the target
  // namespace ('t') contains label-dependent features

  if (m_isFirstTarget) {
    // the first target-side feature => create namespace 't'
    m_isFirstTarget = false;
    m_ex->addns('t');
    if (DEBUG) std::cerr << "VW :: Setting target namespace\n";
  }
  return AddFeature(name, value);
}

void VWPredictor::AddLabelIndependentFeatureVector(const FeatureVector &features)
{
  if (m_isFirstSource) {
    // the first feature of a new example => create the source namespace for
    // label-independent features to live in
    m_isFirstSource = false;
    m_ex->finish();
    m_ex->addns('s');
    if (DEBUG) std::cerr << "VW :: Setting source namespace\n";
  }

  // add each feature index using this "low level" call to VW
  for (FeatureVector::const_iterator it = features.begin(); it != features.end(); it++)
    m_ex->addf(it->first, it->second);
}

void VWPredictor::AddLabelDependentFeatureVector(const FeatureVector &features)
{
  if (m_isFirstTarget) {
    // the first target-side feature => create namespace 't'
    m_isFirstTarget = false;
    m_ex->addns('t');
    if (DEBUG) std::cerr << "VW :: Setting target namespace\n";
  }

  // add each feature index using this "low level" call to VW
  for (FeatureVector::const_iterator it = features.begin(); it != features.end(); it++)
    m_ex->addf(it->first, it->second);
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
  float loss = m_ex->predict_partial();
  if (DEBUG) std::cerr << "VW :: Predicted loss: " << loss << "\n";
  m_ex->remns(); // remove target namespace
  return loss;
}

FeatureType VWPredictor::AddFeature(const StringPiece &name, float value)
{
  if (DEBUG) std::cerr << "VW :: Adding feature: " << EscapeSpecialChars(name.as_string()) << ":" << value << "\n";
  return std::make_pair(m_ex->addf(EscapeSpecialChars(name.as_string()), value), value);
}

} // namespace Discriminative
