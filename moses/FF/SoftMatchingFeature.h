#pragma once

#include <stdexcept>
#include "moses/Util.h"
#include "moses/Word.h"
#include "StatelessFeatureFunction.h"
#include "moses/TranslationModel/PhraseDictionaryNodeMemory.h"

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

namespace Moses
{

class SoftMatchingFeature : public StatelessFeatureFunction
{
public:
  SoftMatchingFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void EvaluateChart(const ChartHypothesis& hypo,
                             ScoreComponentCollection* accumulator) const;

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const {};
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const {};
  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const {};

  bool Load(const std::string &filePath);

  std::map<Word, std::set<Word> >& Get_Soft_Matches() {
    return m_soft_matches;
  }

  std::map<Word, std::set<Word> >& Get_Soft_Matches_Reverse() {
    return m_soft_matches_reverse;
  }

  const std::string& GetFeatureName(const Word& LHS, const Word& RHS) const;
  void SetParameter(const std::string& key, const std::string& value);

private:
  std::map<Word, std::set<Word> > m_soft_matches; // map LHS of old rule to RHS of new rle
  std::map<Word, std::set<Word> > m_soft_matches_reverse; // map RHS of new rule to LHS of old rule

  typedef std::pair<Word, Word> NonTerminalMapKey;

#if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
  typedef boost::unordered_map<NonTerminalMapKey,
          std::string,
          NonTerminalMapKeyHasher,
          NonTerminalMapKeyEqualityPred> NonTerminalSoftMatchingMap;
#else
  typedef std::map<NonTerminalMapKey, std::string> NonTerminalSoftMatchingMap;
#endif

  mutable NonTerminalSoftMatchingMap m_soft_matching_cache;

#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::shared_mutex m_accessLock;
#endif

};

}

