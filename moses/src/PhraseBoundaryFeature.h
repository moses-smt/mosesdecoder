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
  PhraseBoundaryState(const Word* sourceWord, const Word* targetWord) :
     m_sourceWord(sourceWord), m_targetWord(targetWord) {}
  const Word* GetSourceWord() const {return m_sourceWord;}
  const Word* GetTargetWord() const {return m_targetWord;}
  virtual int Compare(const FFState& other) const;


private:
  const Word* m_sourceWord;
  const Word* m_targetWord;
};


/**
 * Concatenations of factors on boundaries of phrases.
 **/
class PhraseBoundaryFeature : public StatefulFeatureFunction {
public:
  PhraseBoundaryFeature(const FactorList& sourceFactors, const FactorList& targetFactors);

  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  virtual const FFState* EmptyHypothesisState(const InputType &) const;

  virtual FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state,
                          ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateChart( const ChartHypothesis& /* cur_hypo */,
                                  int /* featureID */,
                                  ScoreComponentCollection* ) const
                                  {
                                    abort();
                                  }
  
  void SetSparseProducerWeight(float weight) { m_sparseProducerWeight = weight; }
  float GetSparseProducerWeight() const { return m_sparseProducerWeight; }
  
private:
  void AddFeatures(
    const Word* leftWord, const Word* rightWord, const FactorList& factors, 
    const std::string& side, ScoreComponentCollection* scores) const ;
  FactorList m_sourceFactors;
  FactorList m_targetFactors;
  float m_sparseProducerWeight;
};

}


#endif
