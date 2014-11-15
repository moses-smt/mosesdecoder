#ifndef MERT_SEMPOSSCORER_H_
#define MERT_SEMPOSSCORER_H_

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <boost/scoped_ptr.hpp>

#include "Scorer.h"

// NOTE: This header should be included in .cpp file
// because SemposScorer wants to know what actual SemposOverlapping type is
// when we implement the scorer in .cpp file.
// However, currently SemposScorer uses a bunch of typedefs, which are
// used in SemposScorer as well as inherited SemposOverlapping classes.
#include "SemposOverlapping.h"
#include "StatisticsBasedScorer.h"

namespace MosesTuning
{


/**
 * This class represents sempos based metrics.
 */
class SemposScorer: public StatisticsBasedScorer
{
public:
  explicit SemposScorer(const std::string& config);
  ~SemposScorer();

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sindex, const std::string& text, ScoreStats& entry);
  virtual std::size_t NumberOfScores() const {
    return m_ovr->NumberOfScores();
  }
  virtual float calculateScore(const std::vector<ScoreStatsType>& comps) const {
    return m_ovr->calculateScore(comps);
  }

  bool EnableDebug() const {
    return m_enable_debug;
  }

  float weight(int item) const;

private:
  boost::scoped_ptr<SemposOverlapping> m_ovr;
  std::vector<std::vector<sentence_t> > m_ref_sentences;

  typedef std::map<std::string, int> encoding_t;
  typedef encoding_t::iterator encoding_it;

  encoding_t m_semposMap;
  encoding_t m_stringMap;
  bool m_enable_debug;

  void splitSentence(const std::string& sentence, str_sentence_t& splitSentence);
  void encodeSentence(const str_sentence_t& sentence, sentence_t& encodedSentence);
  int encodeString(const std::string& str);
  int encodeSempos(const std::string& sempos);

  std::map<int, float> weightsMap;

  void loadWeights(const std::string& weightsfile);

  // no copying allowed.
  SemposScorer(const SemposScorer&);
  SemposScorer& operator=(const SemposScorer&);
};

}

#endif  // MERT_SEMPOSSCORER_H_
