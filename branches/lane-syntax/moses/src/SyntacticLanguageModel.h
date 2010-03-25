//

#ifndef moses_SyntacticLanguageModel_h
#define moses_SyntacticLanguageModel_h

#include "FeatureFunction.h"

namespace Moses
{
  
  class SyntacticLanguageModel : public StatefulFeatureFunction {

  public:

    SyntacticLanguageModel(const std::vector<std::string>& filePaths,
			   const std::vector<float>& weights);

    size_t GetNumScoreComponents() const;
    std::string GetScoreProducerDescription() const;
    std::string GetScoreProducerWeightShortName() const;

    const FFState* EmptyHypothesisState() const;

    FFState* Evaluate(const Hypothesis& cur_hypo,
		      const FFState* prev_state,
		      ScoreComponentCollection* accumulator) const;

  private:

    size_t m_NumScoreComponents;

  };


}

#endif
