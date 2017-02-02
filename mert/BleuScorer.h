#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "Ngram.h"
#include "Reference.h"
#include "ScopedVector.h"
#include "ScoreData.h"
#include "StatisticsBasedScorer.h"
#include "Types.h"

namespace MosesTuning
{

const size_t kBleuNgramOrder = 4;

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
  virtual statscore_t calculateScore(const std::vector<ScoreStatsType>& comps) const;
  virtual std::size_t NumberOfScores() const {
    return 2 * kBleuNgramOrder + 1;
  }

  void CalcBleuStats(const Reference& ref, const std::string& text, ScoreStats& entry) const;

  int CalcReferenceLength(const Reference& ref, std::size_t length) const;

  ReferenceLengthType GetReferenceLengthType() const {
    return m_ref_length_type;
  }

  void SetReferenceLengthType(ReferenceLengthType type) {
    m_ref_length_type = type;
  }

  const std::vector<Reference*>& GetReferences() const {
    return m_references.get();
  }

  virtual float getReferenceLength(const std::vector<ScoreStatsType>& totals) const {
    return totals[kBleuNgramOrder*2];
  }

  /**
   * Count the ngrams of each type, up to the given length in the input line.
   */
  size_t CountNgrams(const std::string& line, NgramCounts& counts, unsigned int n, bool is_testing=false) const;

  void DumpCounts(std::ostream* os, const NgramCounts& counts) const;

  // NOTE: this function is used for unit testing.
  bool OpenReferenceStream(std::istream* is, std::size_t file_id);

  void ProcessReferenceLine(const std::string& line, Reference* ref) const;

  bool GetNextReferenceFromStreams(std::vector<boost::shared_ptr<std::ifstream> >& referenceStreams, Reference& ref) const;

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

}

