#ifndef moses_WordTranslationFeature_h
#define moses_WordTranslationFeature_h

#include <string>
#include <boost/unordered_set.hpp>

#include "moses/FactorCollection.h"
#include "moses/Sentence.h"
#include "FFState.h"
#include "StatelessFeatureFunction.h"

namespace Moses
{

/** Sets the features for word translation
 */
class WordTranslationFeature : public StatelessFeatureFunction
{

  typedef std::map< char, short > CharHash;
  typedef std::vector< boost::unordered_set<std::string> > DocumentVector;

private:
  boost::unordered_set<std::string> m_vocabSource;
  boost::unordered_set<std::string> m_vocabTarget;
  DocumentVector m_vocabDomain;
  FactorType m_factorTypeSource;
  FactorType m_factorTypeTarget;
  bool m_unrestricted;
  bool m_simple;
  bool m_sourceContext;
  bool m_targetContext;
  bool m_domainTrigger;
  bool m_ignorePunctuation;
  CharHash m_punctuationHash;
  std::string m_filePathSource;
  std::string m_filePathTarget;

public:
  WordTranslationFeature(const std::string &line);

  void SetParameter(const std::string& key, const std::string& value);
  bool IsUseable(const FactorMask &mask) const;

  void Load();

  const FFState* EmptyHypothesisState(const InputType &) const {
    return new DummyState();
  }

  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const;

  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const;
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}
  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const
  {}

};

}

#endif // moses_WordTranslationFeature_h
