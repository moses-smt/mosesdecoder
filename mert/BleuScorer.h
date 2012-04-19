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
  enum ReferenceLengthType {
    AVERAGE,
    CLOSEST,
    SHORTEST
  };

  explicit BleuScorer(const string& config = "");
  ~BleuScorer();

  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
  virtual float calculateScore(const vector<int>& comps) const;
  virtual size_t NumberOfScores() const { return 2 * kBleuNgramOrder + 1; }

  int CalcReferenceLength(size_t sentence_id, size_t length);

  ReferenceLengthType GetReferenceLengthType() const { return m_ref_length_type; }
  void SetReferenceLengthType(ReferenceLengthType type) { m_ref_length_type = type; }

  const std::vector<Reference*>& GetReferences() const { return m_references.get(); }

  /**
   * Count the ngrams of each type, up to the given length in the input line.
   */
  size_t CountNgrams(const string& line, NgramCounts& counts, unsigned int n);

  void DumpCounts(std::ostream* os, const NgramCounts& counts) const;

  bool OpenReference(const char* filename, size_t file_id);

  // NOTE: this function is used for unit testing.
  bool OpenReferenceStream(std::istream* is, size_t file_id);

private:
  ReferenceLengthType m_ref_length_type;

  // reference translations.
  ScopedVector<Reference> m_references;

  // no copying allowed
  BleuScorer(const BleuScorer&);
  BleuScorer& operator=(const BleuScorer&);
};

/** Computes sentence-level BLEU+1 score.
 * This function is used in PRO.
 */
float sentenceLevelBleuPlusOne(const vector<float>& stats);

#endif  // MERT_BLEU_SCORER_H_
