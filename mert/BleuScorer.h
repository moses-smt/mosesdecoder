#ifndef MERT_BLEU_SCORER_H_
#define MERT_BLEU_SCORER_H_

#include <ostream>
#include <string>
#include <vector>

#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"
#include "ScopedVector.h"

using namespace std;

/**
 * Bleu scoring
 */
class BleuScorer: public StatisticsBasedScorer
{
public:
  explicit BleuScorer(const string& config = "");
  ~BleuScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
  virtual float calculateScore(const vector<int>& comps) const;
  virtual size_t NumberOfScores() const { return 2 * kLENGTH + 1; }

private:
  enum ReferenceLengthType {
    AVERAGE,
    SHORTEST,
    CLOSEST
  };

  /**
   * A NgramCounts is a key-value store.
   * Clients don't have to worry about the actual implementation
   * since this type is used in internal only.
   */
  class NgramCounts;

  /**
   * Count the ngrams of each type, up to the given length in the input line.
   */
  size_t countNgrams(const string& line, NgramCounts& counts, unsigned int n);

  void dump_counts(std::ostream* os, const NgramCounts& counts) const;

  // For calculating effective reference length.
  void CalcAverage(size_t sentence_id,
                   vector<ScoreStatsType>& stats) const;
  void CalcClosest(size_t sentence_id, size_t length,
                   vector<ScoreStatsType>& stats) const;
  void CalcShortest(size_t sentence_id,
                    vector<ScoreStatsType>& stats) const;

  const int kLENGTH;
  ReferenceLengthType m_ref_length_type;

  // data extracted from reference files
  ScopedVector<NgramCounts> m_ref_counts;
  vector<vector<size_t> > m_ref_lengths;

  // no copying allowed
  BleuScorer(const BleuScorer&);
  BleuScorer& operator=(const BleuScorer&);
};

#endif  // MERT_BLEU_SCORER_H_
