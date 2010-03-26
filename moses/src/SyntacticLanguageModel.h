//

#ifndef moses_SyntacticLanguageModel_h
#define moses_SyntacticLanguageModel_h

#include "FeatureFunction.h"


class HModel;
class OModel;

namespace Moses
{

  template <class MH, class MO> class SyntacticLanguageModelFiles;
  
  class SyntacticLanguageModel : public StatefulFeatureFunction {

  public:

    SyntacticLanguageModel(const std::vector<std::string>& filePaths,
			   const std::vector<float>& weights,
			   const FactorType factorType,
			   const size_t beamWidth);

    ~SyntacticLanguageModel();

    size_t GetNumScoreComponents() const;
    std::string GetScoreProducerDescription() const;
    std::string GetScoreProducerWeightShortName() const;

    const FFState* EmptyHypothesisState() const;

    FFState* Evaluate(const Hypothesis& cur_hypo,
		      const FFState* prev_state,
		      ScoreComponentCollection* accumulator) const;

  private:

    const size_t m_NumScoreComponents;
    SyntacticLanguageModelFiles<HModel,OModel>* m_files;
    const FactorType m_factorType;
    const size_t m_beamWidth;

  };


}

#endif
