#ifndef moses_TargetBigramFeature_h
#define moses_TargetBigramFeature_h

#include <string>
#include <map>
#include <boost/unordered_set.hpp>

#include "moses/FF/FFState.h"
#include "StatefulFeatureFunction.h"
#include "moses/FactorCollection.h"
#include "moses/Word.h"

namespace Moses
{

class TargetBigramState : public FFState
{
public:
  TargetBigramState(const Word& word): m_word(word) {}
  const Word& GetWord() const {
    return m_word;
  }
  virtual int Compare(const FFState& other) const;

private:
  Word m_word;
};

/** Sets the features of observed bigrams.
 */
class TargetBigramFeature : public StatefulFeatureFunction
{
public:
  TargetBigramFeature(const std::string &line);

  bool Load(const std::string &filePath);

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state,
                            ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateChart( const ChartHypothesis& /* cur_hypo */,
                                  int /* featureID */,
                                  ScoreComponentCollection* ) const {
    abort();
  }

private:
  FactorType m_factorType;
  Word m_bos;
  boost::unordered_set<std::string> m_vocab;
};

}

#endif // moses_TargetBigramFeature_h
