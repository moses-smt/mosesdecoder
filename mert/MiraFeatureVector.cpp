#include <cmath>
#include <iomanip>

#include "MiraFeatureVector.h"

using namespace std;

namespace MosesTuning
{


void MiraFeatureVector::InitSparse(const SparseVector& sparse, size_t ignoreLimit)
{
  vector<size_t> sparseFeats = sparse.feats();
  bool bFirst = true;
  size_t lastFeat = 0;
  m_sparseFeats.reserve(sparseFeats.size());
  m_sparseVals.reserve(sparseFeats.size());
  for(size_t i=0; i<sparseFeats.size(); i++) {
    if (sparseFeats[i] < ignoreLimit) continue;
    size_t feat = m_dense.size() + sparseFeats[i];
    m_sparseFeats.push_back(feat);
    m_sparseVals.push_back(sparse.get(sparseFeats[i]));

    // Check ordered property
    if(bFirst) {
      bFirst = false;
    } else {
      if(lastFeat>=feat) {
        cerr << "Error: Feature indeces must be strictly ascending coming out of SparseVector" << endl;
        exit(1);
      }
    }
    lastFeat = feat;
  }
}

MiraFeatureVector::MiraFeatureVector(const FeatureDataItem& vec)
  : m_dense(vec.dense)
{
  InitSparse(vec.sparse);
}

MiraFeatureVector::MiraFeatureVector(const SparseVector& sparse, size_t num_dense)
{
  m_dense.resize(num_dense);
  //Assume that features with id [0,num_dense) are the dense features
  for (size_t id = 0; id < num_dense; ++id) {
    m_dense[id] = sparse.get(id);
  }
  InitSparse(sparse,num_dense);
}

MiraFeatureVector::MiraFeatureVector(const MiraFeatureVector& other)
  : m_dense(other.m_dense),
    m_sparseFeats(other.m_sparseFeats),
    m_sparseVals(other.m_sparseVals)
{
  if(m_sparseVals.size()!=m_sparseFeats.size()) {
    cerr << "Error: mismatching sparse feat and val sizes" << endl;
    exit(1);
  }
}

MiraFeatureVector::MiraFeatureVector(const vector<ValType>& dense,
                                     const vector<size_t>& sparseFeats,
                                     const vector<ValType>& sparseVals)
  : m_dense(dense),
    m_sparseFeats(sparseFeats),
    m_sparseVals(sparseVals)
{
  if(m_sparseVals.size()!=m_sparseFeats.size()) {
    cerr << "Error: mismatching sparse feat and val sizes" << endl;
    exit(1);
  }
}

ValType MiraFeatureVector::val(size_t index) const
{
  if(index < m_dense.size())
    return m_dense[index];
  else
    return m_sparseVals[index-m_dense.size()];
}

size_t MiraFeatureVector::feat(size_t index) const
{
  if(index < m_dense.size())
    return index;
  else
    return m_sparseFeats[index-m_dense.size()];
}

size_t MiraFeatureVector::size() const
{
  return m_dense.size() + m_sparseVals.size();
}

ValType MiraFeatureVector::sqrNorm() const
{
  ValType toRet = 0.0;
  for(size_t i=0; i<m_dense.size(); i++)
    toRet += m_dense[i]*m_dense[i];
  for(size_t i=0; i<m_sparseVals.size(); i++)
    toRet += m_sparseVals[i] * m_sparseVals[i];
  return toRet;
}

MiraFeatureVector operator-(const MiraFeatureVector& a, const MiraFeatureVector& b)
{
  // Dense subtraction
  vector<ValType> dense;
  if(a.m_dense.size()!=b.m_dense.size()) {
    cerr << "Mismatching dense vectors passed to MiraFeatureVector subtraction" << endl;
    exit(1);
  }
  for(size_t i=0; i<a.m_dense.size(); i++) {
    dense.push_back(a.m_dense[i] - b.m_dense[i]);
  }

  // Sparse subtraction
  size_t i=0;
  size_t j=0;
  vector<ValType> sparseVals;
  vector<size_t> sparseFeats;
  while(i < a.m_sparseFeats.size() && j < b.m_sparseFeats.size()) {

    if(a.m_sparseFeats[i] < b.m_sparseFeats[j]) {
      sparseFeats.push_back(a.m_sparseFeats[i]);
      sparseVals.push_back(a.m_sparseVals[i]);
      i++;
    }

    else if(b.m_sparseFeats[j] < a.m_sparseFeats[i]) {
      sparseFeats.push_back(b.m_sparseFeats[j]);
      sparseVals.push_back(-b.m_sparseVals[j]);
      j++;
    }

    else {
      ValType newVal  = a.m_sparseVals[i] - b.m_sparseVals[j];
      if(abs(newVal)>1e-6) {
        sparseFeats.push_back(a.m_sparseFeats[i]);
        sparseVals.push_back(newVal);
      }
      i++;
      j++;
    }
  }

  while(i<a.m_sparseFeats.size()) {
    sparseFeats.push_back(a.m_sparseFeats[i]);
    sparseVals.push_back(a.m_sparseVals[i]);
    i++;
  }

  while(j<b.m_sparseFeats.size()) {
    sparseFeats.push_back(b.m_sparseFeats[j]);
    sparseVals.push_back(-b.m_sparseVals[j]);
    j++;
  }

  // Create and return vector
  return MiraFeatureVector(dense,sparseFeats,sparseVals);
}

bool operator==(const MiraFeatureVector& a,const MiraFeatureVector& b)
{
  ValType eps = 1e-8;
  //dense features
  if (a.m_dense.size() != b.m_dense.size()) return false;
  for (size_t i = 0; i < a.m_dense.size(); ++i) {
    if (fabs(a.m_dense[i]-b.m_dense[i]) < eps) return false;
  }
  if (a.m_sparseFeats.size() != b.m_sparseFeats.size()) return false;
  for (size_t i = 0; i < a.m_sparseFeats.size(); ++i) {
    if (a.m_sparseFeats[i] != b.m_sparseFeats[i]) return false;
    if (fabs(a.m_sparseVals[i] != b.m_sparseVals[i])) return false;
  }
  return true;

}

ostream& operator<<(ostream& o, const MiraFeatureVector& e)
{
  for(size_t i=0; i<e.size(); i++) {
    if(i>0) o << " ";
    o << e.feat(i) << ":" << e.val(i);
  }
  return o;
}

// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:

}

