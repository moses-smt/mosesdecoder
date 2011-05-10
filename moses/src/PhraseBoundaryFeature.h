#ifndef moses_PhraseBoundaryFeature_h
#define moses_PhraseBoundaryFeature_h

#include <sstream>
#include <string>

#include "FeatureFunction.h"
#include "FFState.h"
#include "Word.h"

namespace Moses
{

class PhraseBoundaryState : public FFState {
public:
  PhraseBoundaryState(const Word* word) : m_word(word) {}
  const Word* GetWord() const {return m_word;}
  virtual int Compare(const FFState& other) const;


private:
  const Word* m_word;
};


/**
 * Concatenations of factors on boundaries of phrases.
 **/
class PhraseBoundaryFeature : public StatefulFeatureFunction {
public:
  PhraseBoundaryFeature(const FactorList& sourceFactors, const FactorList& targetFactors);

  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerWeightShortName() const;
  size_t GetNumInputScores() const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state,
                          ScoreComponentCollection* accumulator) const;

private:
  void AddFeatures(
    const Word* leftWord, const Word* rightWord, const FactorList& factors, 
    const std::string& side, ScoreComponentCollection* scores) const ;
  FactorList m_sourceFactors;
  FactorList m_targetFactors;
};


}


#endif
