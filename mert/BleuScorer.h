#ifndef MERT_BLEU_SCORER_H_
#define MERT_BLEU_SCORER_H_

#include <ostream>
#include <string>
#include <vector>

#include "Types.h"
#include "ScoreData.h"
#include "StatisticsBasedScorer.h"
#include "ScopedVector.h"

namespace MosesTuning
{

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

  explicit BleuScorer(const std::string& config = "");
  ~BleuScorer();

  static std::vector<float> ScoreNbestList(const std::string& scoreFile, const std::string& featureFile);

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);
  virtual statscore_t calculateScore(const std::vector<int>& comps) const;
  virtual std::size_t NumberOfScores() const {
    return 2 * kBleuNgramOrder + 1;
  }

  int CalcReferenceLength(std::size_t sentence_id, std::size_t length);

  ReferenceLengthType GetReferenceLengthType() const {
    return m_ref_length_type;
  }
  void SetReferenceLengthType(ReferenceLengthType type) {
    m_ref_length_type = type;
  }

  const std::vector<Reference*>& GetReferences() const {
    return m_references.get();
  }

  /**
   * Count the ngrams of each type, up to the given length in the input line.
   */
  std::size_t CountNgrams(const std::string& line, NgramCounts& counts, unsigned int n, bool is_testing=false);

  void DumpCounts(std::ostream* os, const NgramCounts& counts) const;

  bool OpenReference(const char* filename, std::size_t file_id);

  // NOTE: this function is used for unit testing.
  virtual bool OpenReferenceStream(std::istream* is, std::size_t file_id);

  //private:
protected:
  ReferenceLengthType m_ref_length_type;

  // reference translations.
  ScopedVector<Reference> m_references;

  // constructor used by subclasses
  BleuScorer(const std::string& name, const std::string& config): StatisticsBasedScorer(name,config) {}

  // no copying allowed
  BleuScorer(const BleuScorer&);
  BleuScorer& operator=(const BleuScorer&);
};

/** Computes sentence-level BLEU+1 score.
 * This function is used in PRO.
 */
float smoothedSentenceBleu
(const std::vector<float>& stats, float smoothing=1.0, bool smoothBP=false);

/** Computes sentence-level BLEU score given a background corpus.
 * This function is used in batch MIRA.
 */
float sentenceLevelBackgroundBleu(const std::vector<float>& sent, const std::vector<float>& bg);

/**
 * Computes plain old BLEU from a vector of stats
 */
float unsmoothedBleu(const std::vector<float>& stats);

}

#endif  // MERT_BLEU_SCORER_H_
