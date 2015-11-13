#ifndef MERT_METEOR_SCORER_H_
#define MERT_METEOR_SCORER_H_

#include <set>
#include <string>
#include <vector>

#ifdef WITH_THREADS
#include <boost/thread/mutex.hpp>
#endif

#include "Types.h"
#include "StatisticsBasedScorer.h"

namespace MosesTuning
{

class ofdstream;
class ifdstream;
class ScoreStats;

/**
 * Meteor scoring
 *
 * https://github.com/mjdenkowski/meteor
 * http://statmt.org/wmt14/pdf/W14-3348.pdf
 *
 * Config:
 * jar - location of meteor-*.jar (meteor-1.5.jar at time of writing)
 * lang - optional language code (default: en)
 * task - optional task (default: tune)
 * m - optional quoted, space delimited module string "exact stem synonym paraphrase" (default varies by language)
 * p - optional quoted, space delimited parameter string "alpha beta gamma delta" (default for tune: "0.5 1.0 0.5 0.5")
 * w - optional quoted, space delimited weight string "w_exact w_stem w_synonym w_paraphrase" (default for tune: "1.0 0.5 0.5 0.5")
 *
 * Usage with mert-moses.pl:
 * --mertargs="--sctype METEOR --scconfig jar:/path/to/meteor-1.5.jar"
 *
 * Usage with mert-moses.pl when using --batch-mira:
 * --batch-mira-args="--sctype METEOR --scconfig jar:/path/to/meteor-1.5.jar"
 */
class MeteorScorer: public StatisticsBasedScorer
{
public:
  explicit MeteorScorer(const std::string& config = "");
  ~MeteorScorer();

  virtual void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  virtual void prepareStats(std::size_t sid, const std::string& text, ScoreStats& entry);

  virtual std::size_t NumberOfScores() const {
    // From edu.cmu.meteor.scorer.MeteorStats
    // tstLen refLen tstFuncWords refFuncWords stage1tstMatchesContent
    // stage1refMatchesContent stage1tstMatchesFunction stage1refMatchesFunction
    // s2tc s2rc s2tf s2rf s3tc s3rc s3tf s3rf s4tc s4rc s4tf s4rf chunks
    // tstwordMatches refWordMatches
    return 23;
  }

  virtual float getReferenceLength(const std::vector<ScoreStatsType>& totals) const {
    // refLen is index 1 (see above stats comment)
    return totals[1];
  }

  virtual float calculateScore(const std::vector<ScoreStatsType>& comps) const;

private:
  // Meteor and process IO
  std::string meteor_jar;
  std::string meteor_lang;
  std::string meteor_task;
  std::string meteor_m;
  std::string meteor_p;
  std::string meteor_w;
  ofdstream* m_to_meteor;
  ifdstream* m_from_meteor;
#ifdef WITH_THREADS
  mutable boost::mutex mtx;
#endif // WITH_THREADS

  // data extracted from reference files
  std::vector<std::string> m_references;
  std::vector<std::vector<std::string> > m_multi_references;

  // no copying allowed
  MeteorScorer(const MeteorScorer&);
  MeteorScorer& operator=(const MeteorScorer&);

};

}

#endif // MERT_METEOR_SCORER_H_
