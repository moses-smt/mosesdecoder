#ifndef moses_GlobalLexicalModel_h
#define moses_GlobalLexicalModel_h

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include "StatelessFeatureFunction.h"
#include "moses/Factor.h"
#include "moses/Phrase.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Range.h"
#include "moses/FactorTypeSet.h"
#include "moses/Sentence.h"

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#endif

namespace Moses
{

class Factor;
class Phrase;
class Hypothesis;
class InputType;

/** Discriminatively trained global lexicon model
 * This is a implementation of Mauser et al., 2009's model that predicts
 * each output word from _all_ the input words. The intuition behind this
 * feature is that it uses context words for disambiguation
 */
class GlobalLexicalModel : public StatelessFeatureFunction
{
  typedef boost::unordered_map< const Word*,
          boost::unordered_map< const Word*, float, UnorderedComparer<Word> , UnorderedComparer<Word> >,
          UnorderedComparer<Word>, UnorderedComparer<Word> > DoubleHash;
  typedef boost::unordered_map< const Word*, float, UnorderedComparer<Word>, UnorderedComparer<Word> > SingleHash;
  typedef std::map< const TargetPhrase*, float > LexiconCache;

  struct ThreadLocalStorage {
    LexiconCache cache;
    const Sentence *input;
  };

private:
  DoubleHash m_hash;
#ifdef WITH_THREADS
  boost::thread_specific_ptr<ThreadLocalStorage> m_local;
#else
  std::auto_ptr<ThreadLocalStorage> m_local;
#endif
  Word *m_bias;

  FactorMask m_inputFactors, m_outputFactors;
  std::vector<FactorType> m_inputFactorsVec, m_outputFactorsVec;
  std::string m_filePath;

  void Load(AllOptions::ptr const& opts);

  float ScorePhrase( const TargetPhrase& targetPhrase ) const;
  float GetFromCacheOrScorePhrase( const TargetPhrase& targetPhrase ) const;

public:
  GlobalLexicalModel(const std::string &line);
  virtual ~GlobalLexicalModel();

  void SetParameter(const std::string& key, const std::string& value);

  void InitializeForInput(ttasksptr const& ttask);

  bool IsUseable(const FactorMask &mask) const;

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedScores) const {
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {
  }

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedScores = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

};

}
#endif
