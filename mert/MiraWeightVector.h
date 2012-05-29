/*
 * MiraWeightVector.h
 * kbmira - k-best Batch MIRA
 *
 * A self-averaging weight-vector. Good for
 * perceptron learning as well.
 * 
 */

#ifndef MERT_MIRA_WEIGHT_VECTOR_H
#define MERT_MIRA_WEIGHT_VECTOR_H

#include <vector>

#include "MiraFeatureVector.h"

using namespace std;

class AvgWeightVector;

class MiraWeightVector {
public:
  /**
   * Constructor, initializes to the zero vector
   */
  MiraWeightVector();

  /**
   * Constructor with provided initial vector
   * \param init Initial feature values
   */
  MiraWeightVector(const vector<ValType>& init); 

  /**
   * Update a the model
   * \param fv  Feature vector to be added to the weights
   * \param tau FV will be scaled by this value before update
   */
  void update(const MiraFeatureVector& fv, float tau);

  /**
   * Perform an empty update (affects averaging)
   */
  void tick();

  /**
   * Score a feature vector according to the model
   * \param fv Feature vector to be scored
   */
  ValType score(const MiraFeatureVector& fv) const;

  /**
   * Squared norm of the weight vector
   */
  ValType sqrNorm() const;

  /**
   * Return an averaged view of this weight vector
   */
  AvgWeightVector avg();

  friend class AvgWeightVector;
  
private:
  /**
   * Updates a weight and lazily updates its total
   */
  void update(size_t index, ValType delta);

  /**
   * Make sure everyone's total is up-to-date
   */
  void fixTotals();

  /**
   * Helper to handle out-of-range weights
   */
  ValType weight(size_t index) const;
  
  vector<ValType> m_weights;
  vector<ValType> m_totals;
  vector<size_t>  m_lastUpdated;
  size_t          m_numUpdates;
};

/**
 * Averaged view of a weight vector
 */
class AvgWeightVector {
public:
  AvgWeightVector(const MiraWeightVector& wv);
  ValType score(const MiraFeatureVector& fv) const;
  ValType weight(size_t index) const;
  size_t size() const;
private:
  const MiraWeightVector& m_wv;
};


#endif // MERT_WEIGHT_VECTOR_H

// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:
