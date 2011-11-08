#ifndef moses_PhrasePairFeature_h
#define moses_PhrasePairFeature_h

#include "Factor.h"
#include "FeatureFunction.h"

namespace Moses {

/**
  * Phrase pair feature, as in Watanabe et al. Uses alignment info.
  **/
class PhrasePairFeature: public StatelessFeatureFunction {
  public:
    PhrasePairFeature(FactorType sourceFactorId, FactorType targetFactorId);

    virtual void Evaluate(
      const TargetPhrase& cur_hypo,
      ScoreComponentCollection* accumulator) const;

    //NB: Should really precompute this feature, but don't have
    //good hooks to do this.
    virtual bool ComputeValueInTranslationOption() const;

    
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumInputScores() const;


  private:
    FactorType m_sourceFactorId;
    FactorType m_targetFactorId;


};


}


#endif
