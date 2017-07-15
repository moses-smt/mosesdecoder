#ifndef moses_PhraseBoundaryFeature_h
#define moses_PhraseBoundaryFeature_h

#include <stdexcept>
#include <sstream>
#include <string>

#include "StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include "moses/Word.h"

namespace Moses
{

class PhraseBoundaryState : public FFState
{
public:
  PhraseBoundaryState(const Word* sourceWord, const Word* targetWord) :
    m_sourceWord(sourceWord), m_targetWord(targetWord) {}
  const Word* GetSourceWord() const {
    return m_sourceWord;
  }
  const Word* GetTargetWord() const {
    return m_targetWord;
  }
  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;


private:
  const Word* m_sourceWord;
  const Word* m_targetWord;
};


/**
 * Concatenations of factors on boundaries of phrases.
 **/
class PhraseBoundaryFeature : public StatefulFeatureFunction
{
public:
  PhraseBoundaryFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const;

  virtual const FFState* EmptyHypothesisState(const InputType &) const;

  virtual FFState* EvaluateWhenApplied(const Hypothesis& cur_hypo, const FFState* prev_state,
                                       ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateWhenApplied( const ChartHypothesis& /* cur_hypo */,
                                        int /* featureID */,
                                        ScoreComponentCollection* ) const {
    throw std::logic_error("PhraseBoundaryState not supported in chart decoder, yet");
  }

  void SetParameter(const std::string& key, const std::string& value);

private:
  void AddFeatures(
    const Word* leftWord, const Word* rightWord, const FactorList& factors,
    const std::string& side, ScoreComponentCollection* scores) const ;
  FactorList m_sourceFactors;
  FactorList m_targetFactors;
};

}


#endif
