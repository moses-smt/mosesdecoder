/*
 * CHRFScorer.h
 *
 *  Created on: Dec 28, 2016
 *      Author: pramathur@ebay.com
 */
#pragma once

#ifndef MERT_CHRFSCORER_H_
#define MERT_CHRFSCORER_H_

#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <boost/shared_ptr.hpp>

#include "Ngram.h"
#include "Reference.h"
#include "ScopedVector.h"
#include "ScoreData.h"
#include "StatisticsBasedScorer.h"
#include "Types.h"

namespace MosesTuning {

const size_t CHRFNgramOrder = 6;
class CHRFScorer : public StatisticsBasedScorer{
public:
	enum ReferenceLengthType {
	    AVERAGE,
	    CLOSEST,
	    SHORTEST
	  };

  explicit CHRFScorer(const std::string& config = "");
  ~CHRFScorer();

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);
  virtual statscore_t calculateScore(const std::vector<ScoreStatsType>& comps) const;
  virtual std::size_t NumberOfScores() const {
    return 3*CHRFNgramOrder + 2;
  }

  void CalcCHRFStats(const Reference& ref, const std::string& text, ScoreStats& entry) const;

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
    return totals[CHRFNgramOrder*3+2];
  }

  /**
   * Count the ngrams of each type, up to the given length in the input line.
   */
  size_t CountNgrams(const std::string& line, NgramCounts& counts, unsigned int n, bool is_testing=false) const;

  void DumpCounts(std::ostream* os, const NgramCounts& counts) const;

  // NOTE: this function is also used for unit testing.
  bool OpenReferenceStream(std::istream* is, std::size_t file_id);

  void ProcessReferenceLine(const std::string& line, Reference* ref) const;

  bool GetNextReferenceFromStreams(std::vector<boost::shared_ptr<std::ifstream> >& referenceStreams, Reference& ref) const;

protected:
  ReferenceLengthType m_ref_length_type;
  // reference translations.
  ScopedVector<Reference> m_references;

  // no copying allowed
  CHRFScorer(const CHRFScorer&);
  CHRFScorer& operator=(const CHRFScorer&);


private:
  float m_beta;
  float m_smooth;
  // data extracted from reference files
  std::vector<std::size_t> m_ref_lengths;
  std::vector<std::multiset<int> > m_ref_tokens;


};

} /* namespace MosesTuning */

#endif /* MERT_CHRFSCORER_H_ */
