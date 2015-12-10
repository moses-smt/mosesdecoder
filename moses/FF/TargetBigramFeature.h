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
  size_t hash() const;
  virtual bool operator==(const FFState& other) const;

private:
  Word m_word;
};

/** Sets the features of observed bigrams.
 */
class TargetBigramFeature : public StatefulFeatureFunction
{
public:
  TargetBigramFeature(const std::string &line);

  void Load(AllOptions::ptr const& opts);

  bool IsUseable(const FactorMask &mask) const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* EvaluateWhenApplied(const Hypothesis& cur_hypo, const FFState* prev_state,
                                       ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateWhenApplied( const ChartHypothesis& /* cur_hypo */,
                                        int /* featureID */,
                                        ScoreComponentCollection* ) const {
    throw std::logic_error("TargetBigramFeature not valid in chart decoder");
  }

  void SetParameter(const std::string& key, const std::string& value);

private:
  FactorType m_factorType;
  Word m_bos;
  std::string m_filePath;
  boost::unordered_set<std::string> m_vocab;
};

}

#endif // moses_TargetBigramFeature_h
