#include "Classifier.h"
#include "vw.h"
#include <iostream>

using namespace std;

namespace Discriminative
{

const int EMPTY_LIST = -1;
const int BAD_LIST_POINTER = -2;

VWPredictorFactory::VWPredictorFactory(
    const string &modelFile,
    const string &vwOptions,
    const int poolSize)
{
  m_VWInstance = VW::initialize(VW_DEFAULT_OPTIONS + " -i " + modelFile + vwOptions);

  if (poolSize < 1)
    throw runtime_error("VWPredictorFactory pool size must be greater than zero!");
  int lastFree = EMPTY_LIST;
  if (VWPredictor::DEBUG) std::cerr << "VW :: filling VWPredictor pool: ";
  for (int i = 0; i < poolSize; ++i)
  {
    m_predictors.push_back(new VWPredictor(m_VWInstance, i, VW_DEFAULT_PARSER_OPTIONS + vwOptions));
    m_nextFree.push_back(lastFree);
    lastFree = i;
    if (VWPredictor::DEBUG) std::cerr << ".";
  }
  if (VWPredictor::DEBUG) std::cerr << "done.\n";
  m_firstFree = lastFree;
}

VWPredictorFactory::~VWPredictorFactory()
{
  boost::unique_lock<boost::mutex> lock(m_mutex);
  size_t count = 0;
  int prev = EMPTY_LIST;
  for (int cur = m_firstFree; cur != EMPTY_LIST; cur = m_nextFree[cur])
  {
    if (cur == BAD_LIST_POINTER)
      throw std::runtime_error("VWPredictorFactory::~VWPredictorFactory -- bad free list!");
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
      throw std::runtime_error("VWPredictorFactory::~VWPredictorFactory -- not all consumers were returned to pool at destruction time!");

  for (size_t s = 0; s < m_predictors.size(); ++s)
  {
    delete m_predictors[s];
    m_predictors[s] = NULL;
  }
  m_predictors.clear();
  VW::finish(*m_VWInstance);
}

VWPredictor *VWPredictorFactory::Acquire()
{
  boost::unique_lock<boost::mutex> lock(m_mutex);
  while (m_firstFree == EMPTY_LIST)
    m_cond.wait(lock);

  int free = m_firstFree;
  m_firstFree = m_nextFree[free];
  return m_predictors[free];
}

void VWPredictorFactory::Release(VWPredictor *vwpred)
{
  // use scope block to handle the lock
  {
    boost::unique_lock<boost::mutex> lock(m_mutex);
    int index = vwpred->m_index;

    if (index < 0 || index >= (int)m_predictors.size())
      throw std::runtime_error("bad index at VWPredictorFactory::Release");

    if (vwpred != m_predictors[index])
      throw std::runtime_error("mismatched pointer at VWPredictorFactory::Release");

    m_nextFree[index] = m_firstFree;
    m_firstFree = index;
  }
  // release the semaphore *AFTER* the lock goes out of scope
  m_cond.notify_one();
}

} // namespace Discriminative
