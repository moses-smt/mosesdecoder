#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "Types.h"
#include "ScoreData.h"
#include "StatisticsBasedScorer.h"
#include "ScopedVector.h"
#include "BleuScorer.h"

namespace MosesTuning
{

/**
 * Bleu document scoring
 *
 * Needs xml reference files, and nbest lists where sentences are separated by '\n'
 */
class BleuDocScorer : public BleuScorer
{
public:

  explicit BleuDocScorer(const std::string& config = "");
  ~BleuDocScorer();

  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);
  virtual statscore_t calculateScore(const std::vector<int>& comps) const;

  int CalcReferenceLength(std::size_t doc_id, std::size_t sentence_id, std::size_t length);

  // NOTE: this function is used for unit testing.
  virtual bool OpenReferenceStream(std::istream* is, std::size_t file_id);

private:
  ReferenceLengthType m_ref_length_type;

  // reference translations.
  ScopedVector<ScopedVector<Reference> > m_references;

  // no copying allowed
  BleuDocScorer(const BleuDocScorer&);
  BleuDocScorer& operator=(const BleuDocScorer&);

  std::vector<std::string> splitDoc(const std::string& text);
};

/* /\** Computes sentence-level BLEU+1 score. */
/*  * This function is used in PRO. */
/*  *\/ */
/* float sentenceLevelBleuPlusOne(const std::vector<float>& stats); */

/* /\** Computes sentence-level BLEU score given a background corpus. */
/*  * This function is used in batch MIRA. */
/*  *\/ */
/* float sentenceLevelBackgroundBleu(const std::vector<float>& sent, const std::vector<float>& bg); */

/* /\** */
/*  * Computes plain old BLEU from a vector of stats */
/*  *\/ */
/* float unsmoothedBleu(const std::vector<float>& stats); */

}

