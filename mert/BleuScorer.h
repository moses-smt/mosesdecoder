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

const int kBleuNgramOrder = 4;

class NgramCounts;
class Reference;

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
  virtual size_t NumberOfScores() const { return 2 * kBleuNgramOrder + 1; }

  int CalcReferenceLength(size_t sentence_id, size_t length);

private:
  enum ReferenceLengthType {
    AVERAGE,
    SHORTEST,
    CLOSEST
  };

  /**
   * Count the ngrams of each type, up to the given length in the input line.
   */
  size_t countNgrams(const string& line, NgramCounts& counts, unsigned int n);

  void dump_counts(std::ostream* os, const NgramCounts& counts) const;

  ReferenceLengthType m_ref_length_type;

  // reference translations.
  ScopedVector<Reference> m_references;

  // no copying allowed
  BleuScorer(const BleuScorer&);
  BleuScorer& operator=(const BleuScorer&);
};

#endif  // MERT_BLEU_SCORER_H_
