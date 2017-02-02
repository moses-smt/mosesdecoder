#pragma once

#include <string>
#include <limits>
#include <set>
#include <boost/unordered_map.hpp>
#include "StatelessFeatureFunction.h"
#include "moses/Factor.h"

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

namespace Moses
{

class Model1Vocabulary
{
public:

#define INVALID_ID std::numeric_limits<unsigned>::max() // UINT_MAX
  static const std::string GIZANULL;

  Model1Vocabulary();
  bool Store(const Factor* word, const unsigned id);
  unsigned StoreIfNew(const Factor* word);
  unsigned GetWordID(const Factor* word) const;
  const Factor* GetWord(unsigned id) const;
  void Load(const std::string& fileName);

protected:
  boost::unordered_map<const Factor*, unsigned> m_lookup;
  std::vector< const Factor* > m_vocab;
  const Factor* m_NULL;
};


class Model1LexicalTable
{
public:
  Model1LexicalTable(float floor=1e-7) : m_floor(floor)
  {}

  void Load(const std::string& fileName, const Model1Vocabulary& vcbS, const Model1Vocabulary& vcbT);

  // p( wordT | wordS )
  float GetProbability(const Factor* wordS, const Factor* wordT) const;

protected:
  boost::unordered_map< const Factor*, boost::unordered_map< const Factor*, float > > m_ltable;
  const float m_floor;
};



class Model1Feature : public StatelessFeatureFunction
{
public:
  Model1Feature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedScores) const
  {};

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedScores = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const
  {}

  void EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    ScoreComponentCollection* accumulator) const
  {}

  void EvaluateWhenApplied(
    const ChartHypothesis& cur_hypo,
    ScoreComponentCollection* accumulator) const
  {}

  void CleanUpAfterSentenceProcessing(const InputType& source);

private:
  std::string m_fileNameVcbS;
  std::string m_fileNameVcbT;
  std::string m_fileNameModel1;
  Model1LexicalTable m_model1;
  const Factor* m_emptyWord;
  bool m_skipTargetPunctuation;
  std::set<const Factor*> m_punctuation;
  bool m_is_syntax;

  void Load(AllOptions::ptr const& opts);

  // cache
  mutable boost::unordered_map<const InputType*, boost::unordered_map<const Factor*, float> > m_cache;
#ifdef WITH_THREADS
  // reader-writer lock
  mutable boost::shared_mutex m_accessLock;
#endif
};


}

