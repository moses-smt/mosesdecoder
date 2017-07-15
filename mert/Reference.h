#ifndef MERT_REFERENCE_H_
#define MERT_REFERENCE_H_

#include <algorithm>
#include <climits>
#include <vector>

#include "Ngram.h"

namespace MosesTuning
{


/**
 * Reference class represents reference translations for an output
 * translation used in calculating BLEU score.
 */
class Reference
{
public:
  // for m_length
  typedef std::vector<std::size_t>::iterator iterator;
  typedef std::vector<std::size_t>::const_iterator const_iterator;

  Reference() : m_counts(new NgramCounts) { }
  ~Reference() {
    delete m_counts;
  }

  NgramCounts* get_counts() {
    return m_counts;
  }
  const NgramCounts* get_counts() const {
    return m_counts;
  }

  iterator begin() {
    return m_length.begin();
  }
  const_iterator begin() const {
    return m_length.begin();
  }
  iterator end() {
    return m_length.end();
  }
  const_iterator end() const {
    return m_length.end();
  }

  void push_back(std::size_t len) {
    m_length.push_back(len);
  }

  std::size_t num_references() const {
    return m_length.size();
  }

  int CalcAverage() const;
  int CalcClosest(std::size_t length) const;
  int CalcShortest() const;

  void clear() {
    m_length.clear();
    m_counts->clear();
  }

private:
  NgramCounts* m_counts;

  // multiple reference lengths
  std::vector<std::size_t> m_length;
};

// TODO(tetsuok): fix this function and related stuff.
// "average" reference length should not be calculated at sentence-level unlike "closest".
inline int Reference::CalcAverage() const
{
  int total = 0;
  for (std::size_t i = 0; i < m_length.size(); ++i) {
    total += m_length[i];
  }
  return static_cast<int>(
           static_cast<float>(total) / m_length.size());
}

inline int Reference::CalcClosest(std::size_t length) const
{
  int min_diff = INT_MAX;
  int closest_ref_id = 0; // an index of the closest reference translation
  for (std::size_t i = 0; i < m_length.size(); ++i) {
    const int ref_length = m_length[i];
    const int length_diff = abs(ref_length - static_cast<int>(length));
    const int abs_min_diff = abs(min_diff);
    // Look for the closest reference
    if (length_diff < abs_min_diff) {
      min_diff = ref_length - length;
      closest_ref_id = i;
      // if two references has the same closest length, take the shortest
    } else if (length_diff == abs_min_diff) {
      if (ref_length < static_cast<int>(m_length[closest_ref_id])) {
        closest_ref_id = i;
      }
    }
  }
  return static_cast<int>(m_length[closest_ref_id]);
}

inline int Reference::CalcShortest() const
{
  return *std::min_element(m_length.begin(), m_length.end());
}

}


#endif  // MERT_REFERENCE_H_
