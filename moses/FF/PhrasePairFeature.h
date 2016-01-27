#pragma once

#include <stdexcept>
#include <boost/unordered_set.hpp>

#include "StatelessFeatureFunction.h"
#include "moses/Factor.h"
#include "moses/Sentence.h"

namespace Moses
{

/**
  * Phrase pair feature: complete source/target phrase pair
  **/
class PhrasePairFeature: public StatelessFeatureFunction
{

  typedef std::map< char, short > CharHash;
  typedef std::vector< std::set<std::string> > DocumentVector;

  boost::unordered_set<std::string> m_vocabSource;
  DocumentVector m_vocabDomain;
  FactorType m_sourceFactorId;
  FactorType m_targetFactorId;
  bool m_unrestricted;
  bool m_simple;
  bool m_sourceContext;
  bool m_domainTrigger;
  bool m_ignorePunctuation;
  CharHash m_punctuationHash;
  std::string m_filePathSource;

  inline std::string ReplaceTilde(const StringPiece &str) const {
    std::string out = str.as_string();
    size_t pos = out.find('~');
    while ( pos != std::string::npos ) {
      out.replace(pos,1,"<TILDE>");
      pos = out.find('~',pos);
    }
    return out;
  };

public:
  PhrasePairFeature(const std::string &line);

  void Load(AllOptions::ptr const& opts);
  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const;

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedScores) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }
  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedScores = NULL) const;

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }

  void EvaluateWhenApplied(const ChartHypothesis& hypo,
                           ScoreComponentCollection*) const {
  }


};

}

