/*
 * MiraFeatureVector.h
 * kbmira - k-best Batch MIRA
 *
 * An alternative to the existing SparseVector
 * and FeatureDataItem combo. Should be as memory
 * efficient, and a little more time efficient,
 * and should save me from constantly hacking
 * SparseVector
 */

#ifndef MERT_MIRA_FEATURE_VECTOR_H
#define MERT_MIRA_FEATURE_VECTOR_H

#include <vector>
#include <iostream>

#include "FeatureDataIterator.h"

namespace MosesTuning
{


typedef FeatureStatsType ValType;

class MiraFeatureVector
{
public:
  MiraFeatureVector(const FeatureDataItem& vec);
  MiraFeatureVector(const MiraFeatureVector& other);
  MiraFeatureVector(const std::vector<ValType>& dense,
                    const std::vector<std::size_t>& sparseFeats,
                    const std::vector<ValType>& sparseVals);

  ValType val(std::size_t index) const;
  std::size_t feat(std::size_t index) const;
  std::size_t size() const;
  ValType sqrNorm() const;

  friend MiraFeatureVector operator-(const MiraFeatureVector& a,
                                     const MiraFeatureVector& b);

  friend std::ostream& operator<<(std::ostream& o, const MiraFeatureVector& e);

private:
  std::vector<ValType> m_dense;
  std::vector<std::size_t> m_sparseFeats;
  std::vector<ValType> m_sparseVals;
};

} // namespace

#endif // MERT_FEATURE_VECTOR_H

// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:


