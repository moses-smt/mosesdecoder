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
// VWLibraryPredictConsumerFactory
//

const int EMPTY_LIST = -1;
const int BAD_LIST_POINTER = -2;

VWLibraryPredictConsumerFactory::VWLibraryPredictConsumerFactory(
    const string &modelFile,
    const string &vwOptions,
    const int poolSize)
{
  m_VWInstance = VW::initialize(vwOptions + "-i " + modelFile);

  if (poolSize < 1)
    throw runtime_error("VWLibraryPredictConsumerFactory pool size must be greater than zero!");
  int lastFree = EMPTY_LIST;
  for (int i = 0; i < poolSize; ++i)
  {
    m_consumers.push_back(new VWLibraryPredictConsumer(m_VWInstance, i));
    m_nextFree.push_back(lastFree);
    lastFree = i;
  }
  m_firstFree = lastFree;
}

VWLibraryPredictConsumerFactory::~VWLibraryPredictConsumerFactory()
{
  boost::unique_lock<boost::mutex> lock(m_mutex);
  size_t count = 0;
  int prev = EMPTY_LIST;
  for (int cur = m_firstFree; cur != EMPTY_LIST; cur = m_nextFree[cur])
  {
    if (cur == BAD_LIST_POINTER)
      throw std::runtime_error("VWLibraryPredictConsumerFactory::~VWLibraryPredictConsumerFactory -- bad free list!");
    ++count;
    if (prev == EMPTY_LIST)
      m_firstFree = BAD_LIST_POINTER;
    else
      m_nextFree[prev] = BAD_LIST_POINTER;
    prev = cur;
  }
  if (prev != EMPTY_LIST)
    m_nextFree[prev] = BAD_LIST_POINTER;
  if (count != m_nextFree.size())
      throw std::runtime_error("VWLibraryPredictConsumerFactory::~VWLibraryPredictConsumerFactory -- not all consumers were returned to pool at destruction time!");

  for (size_t s = 0; s < m_consumers.size(); ++s)
  {
    delete m_consumers[s];
    m_consumers[s] = NULL;
  }
  m_consumers.clear();
  VW::finish(*m_VWInstance);
}

VWLibraryPredictConsumer * VWLibraryPredictConsumerFactory::Acquire()
{
  boost::unique_lock<boost::mutex> lock(m_mutex);
  while (m_firstFree == EMPTY_LIST)
    m_cond.wait(lock);

  int free = m_firstFree;
  m_firstFree = m_nextFree[free];
  return m_consumers[free];
}

void VWLibraryPredictConsumerFactory::Release(VWLibraryPredictConsumer * fc)
{
  // use scope block to handle the lock
  {
    boost::unique_lock<boost::mutex> lock(m_mutex);
    int index = fc->m_index;

    if (index < 0 || index >= (int)m_consumers.size())
      throw std::runtime_error("bad index at VWLibraryPredictConsumerFactory::Release");

    if (fc != m_consumers[index])
      throw std::runtime_error("mismatched pointer at VWLibraryPredictConsumerFactory::Release");

    m_nextFree[index] = m_firstFree;
    m_firstFree = index;
  }
  // release the semaphore *AFTER* the lock goes out of scope
  m_cond.notify_one();
}

// 
// VWLibraryConsumer
//

void VWLibraryConsumer::SetNamespace(char ns, bool shared)
{
  if (!m_shared) {
    m_ex->remns();
  }

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
  m_shared = true; // avoid removing an empty namespace in next call of SetNamespace
  m_ex->clear_features();
}

void VWLibraryConsumer::Finish()
{
  if (m_sharedVwInstance)
    m_VWInstance = NULL;
  else
    VW::finish(*m_VWInstance);
}

VWLibraryConsumer::~VWLibraryConsumer()
{
  delete m_ex;
  if (!m_sharedVwInstance)
    VW::finish(*m_VWInstance);
}

//
// VWLibraryTrainConsumer
//

VWLibraryTrainConsumer::VWLibraryTrainConsumer(const string &modelFile, const string &vwOptions)
{
  m_shared = true;
  m_VWInstance = VW::initialize(vwOptions + " -f " + modelFile);
  m_sharedVwInstance = false;
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

VWLibraryPredictConsumer::VWLibraryPredictConsumer(const string &modelFile, const string &vwOptions)
{
  m_shared = true;
  m_VWInstance = VW::initialize(vwOptions + " -i " + modelFile);
  m_sharedVwInstance = false;
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

VWLibraryPredictConsumer::VWLibraryPredictConsumer(vw * instance, int index)
{
  m_VWInstance = instance;
  m_sharedVwInstance = true;
  m_ex = new ::ezexample(m_VWInstance, false);
  m_shared = true;
  m_index = index;
}

} // namespace PSD
