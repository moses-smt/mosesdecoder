#include "Classifier.h"
#include "vw.h"
#include "../moses/Util.h"
#include <iostream>

namespace Discriminative
{

ClassifierFactory::ClassifierFactory(const std::string &modelFile, const std::string &vwOptions)
  : m_vwOptions(vwOptions), m_train(false)
{
  m_VWInstance = VW::initialize(VW_DEFAULT_OPTIONS + " -i " + modelFile + vwOptions);
}

ClassifierFactory::ClassifierFactory(const std::string &modelFilePrefix)
  : m_lastId(0), m_train(true)
{
  if (modelFilePrefix.size() > 3 && modelFilePrefix.substr(modelFilePrefix.size() - 3, 3) == ".gz") {
    m_modelFilePrefix = modelFilePrefix.substr(0, modelFilePrefix.size() - 3);
    m_gzip = true;
  } else {
    m_modelFilePrefix = modelFilePrefix;
    m_gzip = false;
  }
}

ClassifierFactory::~ClassifierFactory()
{
  if (! m_train)
    VW::finish(*m_VWInstance);
}

ClassifierFactory::ClassifierPtr ClassifierFactory::operator()()
{
  if (m_train) {
    boost::unique_lock<boost::mutex> lock(m_mutex); // avoid possible race for m_lastId
    return ClassifierFactory::ClassifierPtr(
             new VWTrainer(m_modelFilePrefix + "." + Moses::SPrint(m_lastId++) + (m_gzip ? ".gz" : "")));
  } else {
    return ClassifierFactory::ClassifierPtr(
             new VWPredictor(m_VWInstance, VW_DEFAULT_PARSER_OPTIONS + m_vwOptions));
  }
}

}
