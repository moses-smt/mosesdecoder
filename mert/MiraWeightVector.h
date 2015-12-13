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
#include <iostream>

#include "MiraFeatureVector.h"

namespace MosesTuning
{


class AvgWeightVector;

class MiraWeightVector
{
public:
  /**
   * Constructor, initializes to the zero vector
   */
  MiraWeightVector();

  /**
   * Constructor with provided initial vector
   * \param init Initial feature values
   */
  MiraWeightVector(const std::vector<ValType>& init);

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

  /**
    * Convert to sparse vector, interpreting all features as sparse. Only used by hgmira.
   **/
  void ToSparse(SparseVector* sparse, size_t denseSize) const;

  friend class AvgWeightVector;

  friend std::ostream& operator<<(std::ostream& o, const MiraWeightVector& e);

private:
  /**
   * Updates a weight and lazily updates its total
   */
  void update(std::size_t index, ValType delta);

  /**
   * Make sure everyone's total is up-to-date
   */
  void fixTotals();

  /**
   * Helper to handle out-of-range weights
   */
  ValType weight(std::size_t index) const;

  std::vector<ValType> m_weights;
  std::vector<ValType> m_totals;
  std::vector<std::size_t> m_lastUpdated;
  std::size_t m_numUpdates;
};

/**
 * Averaged view of a weight vector
 */
class AvgWeightVector
{
public:
  AvgWeightVector(const MiraWeightVector& wv);
  ValType score(const MiraFeatureVector& fv) const;
  ValType weight(std::size_t index) const;
  std::size_t size() const;
  void ToSparse(SparseVector* sparse, size_t num_dense) const;
private:
  const MiraWeightVector& m_wv;
};



// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:

}
#endif // MERT_WEIGHT_VECTOR_H
