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
#include <stddef.h>

#include "FeatureDataIterator.h"
#include "ScoreDataIterator.h"
#include "MiraFeatureVector.h"

namespace MosesTuning
{


// Start with these abstract classes

class HypPackEnumerator
{
public:
  virtual ~HypPackEnumerator() {}

  virtual void reset() = 0;
  virtual bool finished() = 0;
  virtual void next() = 0;

  virtual std::size_t cur_id() = 0;
  virtual std::size_t cur_size() = 0;
  virtual std::size_t num_dense() const = 0;
  virtual const MiraFeatureVector& featuresAt(std::size_t i) = 0;
  virtual const ScoreDataItem& scoresAt(std::size_t i) = 0;
};

// Instantiation that streams from disk
// Low-memory, low-speed, sequential access
class StreamingHypPackEnumerator : public HypPackEnumerator
{
public:
  StreamingHypPackEnumerator(std::vector<std::string> const& featureFiles,
                             std::vector<std::string> const& scoreFiles);

  virtual std::size_t num_dense() const;

  virtual void reset();
  virtual bool finished();
  virtual void next();

  virtual std::size_t cur_id();
  virtual std::size_t cur_size();
  virtual const MiraFeatureVector& featuresAt(std::size_t i);
  virtual const ScoreDataItem& scoresAt(std::size_t i);

private:
  void prime();
  std::size_t m_num_lists;
  std::size_t m_sentenceId;
  std::vector<std::string> m_featureFiles;
  std::vector<std::string> m_scoreFiles;

  bool m_primed;
  int m_iNumDense;
  std::vector<FeatureDataIterator>  m_featureDataIters;
  std::vector<ScoreDataIterator>    m_scoreDataIters;
  std::vector<std::pair<std::size_t,std::size_t> > m_current_indexes;
  std::vector<MiraFeatureVector>    m_current_featureVectors;
};

// Instantiation that reads into memory
// High-memory, high-speed, random access
// (Actually randomizes with each call to reset)
class RandomAccessHypPackEnumerator : public HypPackEnumerator
{
public:
  RandomAccessHypPackEnumerator(std::vector<std::string> const& featureFiles,
                                std::vector<std::string> const& scoreFiles,
                                bool no_shuffle);

  virtual std::size_t num_dense() const;

  virtual void reset();
  virtual bool finished();
  virtual void next();

  virtual std::size_t cur_id();
  virtual std::size_t cur_size();
  virtual const MiraFeatureVector& featuresAt(std::size_t i);
  virtual const ScoreDataItem& scoresAt(std::size_t i);

private:
  bool m_no_shuffle;
  std::size_t m_cur_index;
  std::size_t m_num_dense;
  std::vector<std::size_t> m_indexes;
  std::vector<std::vector<MiraFeatureVector> > m_features;
  std::vector<std::vector<ScoreDataItem> > m_scores;
};

}

#endif // MERT_HYP_PACK_COLLECTION_H

// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:
