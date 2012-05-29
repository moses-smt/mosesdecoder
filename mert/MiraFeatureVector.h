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

#include "FeatureDataIterator.h"

using namespace std;

typedef FeatureStatsType ValType;

class MiraFeatureVector {
public:
  MiraFeatureVector(const FeatureDataItem& vec);
  MiraFeatureVector(const MiraFeatureVector& other);
  MiraFeatureVector(const vector<ValType>& dense,
                    const vector<size_t>& sparseFeats,
                    const vector<ValType>& sparseVals);
  
  ValType val(size_t index) const;
  size_t feat(size_t index) const;
  size_t size() const;
  ValType sqrNorm() const;
  
  friend MiraFeatureVector operator-(const MiraFeatureVector& a,
                                     const MiraFeatureVector& b);
  
private:
  vector<ValType> m_dense;
  vector<size_t>  m_sparseFeats;
  vector<ValType> m_sparseVals;
};

#endif // MERT_FEATURE_VECTOR_H

// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:
