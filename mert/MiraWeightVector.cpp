#include "MiraWeightVector.h"

#include <cmath>

using namespace std;

namespace MosesTuning
{


/**
 * Constructor, initializes to the zero vector
 */
MiraWeightVector::MiraWeightVector()
  : m_weights(),
    m_totals(),
    m_lastUpdated()
{
  m_numUpdates = 0;
}

/**
 * Constructor with provided initial vector
 * \param init Initial feature values
 */
MiraWeightVector::MiraWeightVector(const vector<ValType>& init)
  : m_weights(init),
    m_totals(init),
    m_lastUpdated(init.size(), 0)
{
  m_numUpdates = 0;
}

/**
 * Update a the model
 * \param fv  Feature vector to be added to the weights
 * \param tau FV will be scaled by this value before update
 */
void MiraWeightVector::update(const MiraFeatureVector& fv, float tau)
{
  m_numUpdates++;
  for(size_t i=0; i<fv.size(); i++) {
    update(fv.feat(i), fv.val(i)*tau);
  }
}

/**
 * Perform an empty update (affects averaging)
 */
void MiraWeightVector::tick()
{
  m_numUpdates++;
}

/**
 * Score a feature vector according to the model
 * \param fv Feature vector to be scored
 */
ValType MiraWeightVector::score(const MiraFeatureVector& fv) const
{
  ValType toRet = 0.0;
  for(size_t i=0; i<fv.size(); i++) {
    toRet += weight(fv.feat(i)) * fv.val(i);
  }
  return toRet;
}

/**
 * Return an averaged view of this weight vector
 */
AvgWeightVector MiraWeightVector::avg()
{
  this->fixTotals();
  return AvgWeightVector(*this);
}

/**
 * Updates a weight and lazily updates its total
 */
void MiraWeightVector::update(size_t index, ValType delta)
{

  // Handle previously unseen weights
  while(index>=m_weights.size()) {
    m_weights.push_back(0.0);
    m_totals.push_back(0.0);
    m_lastUpdated.push_back(0);
  }

  // Book keeping for w = w + delta
  m_totals[index] += (m_numUpdates - m_lastUpdated[index]) * m_weights[index] + delta;
  m_weights[index] += delta;
  m_lastUpdated[index] = m_numUpdates;
}

void MiraWeightVector::ToSparse(SparseVector* sparse, size_t denseSize) const
{
  for (size_t i = 0; i < m_weights.size(); ++i) {
    if(abs(m_weights[i])>1e-8) {
      if (i < denseSize) {
        sparse->set(i,m_weights[i]);
      } else {
        //The ids in MiraFeatureVector/MiraWeightVector for sparse features
        //need to be translated when converting back to SparseVector.
        sparse->set(i-denseSize, m_weights[i]);
      }
    }
  }
}

/**
 * Make sure everyone's total is up-to-date
 */
void MiraWeightVector::fixTotals()
{
  for(size_t i=0; i<m_weights.size(); i++) update(i,0);
}

/**
 * Helper to handle out of range weights
 */
ValType MiraWeightVector::weight(size_t index) const
{
  if(index < m_weights.size()) {
    return m_weights[index];
  } else {
    return 0;
  }
}

ValType MiraWeightVector::sqrNorm() const
{
  ValType toRet = 0;
  for(size_t i=0; i<m_weights.size(); i++) {
    toRet += weight(i) * weight(i);
  }
  return toRet;
}

AvgWeightVector::AvgWeightVector(const MiraWeightVector& wv)
  :m_wv(wv)
{}

ostream& operator<<(ostream& o, const MiraWeightVector& e)
{
  for(size_t i=0; i<e.m_weights.size(); i++) {
    if(abs(e.m_weights[i])>1e-8) {
      if(i>0) o << " ";
      o << i << ":" << e.m_weights[i];
    }
  }
  return o;
}

ValType AvgWeightVector::weight(size_t index) const
{
  if(m_wv.m_numUpdates==0) return m_wv.weight(index);
  else {
    if(index < m_wv.m_totals.size()) {
      return m_wv.m_totals[index] / m_wv.m_numUpdates;
    } else {
      return 0;
    }
  }
}

ValType AvgWeightVector::score(const MiraFeatureVector& fv) const
{
  ValType toRet = 0.0;
  for(size_t i=0; i<fv.size(); i++) {
    toRet += weight(fv.feat(i)) * fv.val(i);
  }
  return toRet;
}

size_t AvgWeightVector::size() const
{
  return m_wv.m_weights.size();
}

void AvgWeightVector::ToSparse(SparseVector* sparse, size_t denseSize) const
{
  for (size_t i = 0; i < size(); ++i) {
    ValType w = weight(i);
    if(abs(w)>1e-8) {
      if (i < denseSize) {
        sparse->set(i,w);
      } else {
        //The ids in MiraFeatureVector/MiraWeightVector for sparse features
        //need to be translated when converting back to SparseVector.
        sparse->set(i-denseSize, w);
      }
    }
  }
}

// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:
}

