/*
 * HypPackCollection.h
 * kbmira - k-best Batch MIRA
 *
 * Abstracts away the mess of iterating through multiple
 * collections of k-best lists, as well as deduping
 */

#ifndef MERT_HYP_PACK_COLLECTION_H
#define MERT_HYP_PACK_COLLECTION_H

#include <string>
#include <vector>
#include <utility>

#include "FeatureDataIterator.h"
#include "ScoreDataIterator.h"

using namespace std;

// Start with these abstract classes

class HypPackEnumerator {
public:
  virtual void reset() = 0;
  virtual bool finished() = 0;
  virtual void next() = 0;

  virtual size_t cur_size() = 0;
  virtual size_t num_dense() const = 0;
  virtual const FeatureDataItem& featuresAt(size_t i) = 0;
  virtual const ScoreDataItem& scoresAt(size_t i) = 0;
};

// Instantiation that streams from disk
// Low-memory, low-speed, sequential access
class StreamingHypPackEnumerator : public HypPackEnumerator {
public:
  StreamingHypPackEnumerator(vector<string> const& featureFiles,
                             vector<string> const& scoreFiles
                             );

  virtual size_t num_dense() const;
  
  virtual void reset();
  virtual bool finished();
  virtual void next();

  virtual size_t cur_size();
  virtual const FeatureDataItem& featuresAt(size_t i);
  virtual const ScoreDataItem& scoresAt(size_t i);
  
private:
  void prime();
  size_t m_num_lists;
  size_t m_sentenceId;
  vector<string> m_featureFiles;
  vector<string> m_scoreFiles;

  bool m_primed;
  int m_iNumDense;
  vector<FeatureDataIterator>  m_featureDataIters;
  vector<ScoreDataIterator>    m_scoreDataIters;
  vector<pair<size_t,size_t> > m_current_indexes;
};

// Instantiation that reads into memory
// High-memory, high-speed, random access
// (Actually randomizes with each call to reset)
class RandomAccessHypPackEnumerator : public HypPackEnumerator {
public:
  RandomAccessHypPackEnumerator(vector<string> const& featureFiles,
                                vector<string> const& scoreFiles,
                                bool no_shuffle);

  virtual size_t num_dense() const;
  
  virtual void reset();
  virtual bool finished();
  virtual void next();

  virtual size_t cur_size();
  virtual const FeatureDataItem& featuresAt(size_t i);
  virtual const ScoreDataItem& scoresAt(size_t i);

private:
  bool m_no_shuffle;
  size_t m_cur_index;
  size_t m_num_dense;
  vector<size_t> m_indexes;
  vector<vector<FeatureDataItem> > m_features;
  vector<vector<ScoreDataItem> > m_scores;
};

#endif // MERT_HYP_PACK_COLLECTION_H

// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:
