//

#include "SyntacticLanguageModel.h"
#include "StaticData.h"

namespace Moses
{

  SyntacticLanguageModel::SyntacticLanguageModel(const std::vector<std::string>& filePath,
						 const std::vector<float>& weights) 
    : m_NumScoreComponents(weights.size()) {

    const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
    const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);

    //    cerr << "Loading syntactic language model..." << std::endl;

  }


  size_t SyntacticLanguageModel::GetNumScoreComponents() const {
    return m_NumScoreComponents;
  }

  std::string SyntacticLanguageModel::GetScoreProducerDescription() const {
    return "Syntactic Language Model";
  }

  std::string SyntacticLanguageModel::GetScoreProducerWeightShortName() const {
    return "slm";
  }

  const FFState* SyntacticLanguageModel::EmptyHypothesisState() const {

    cerr << "Constructing empty syntactic language model state" << std::endl;

    return NULL;
  }


  FFState* SyntacticLanguageModel::Evaluate(const Hypothesis& cur_hypo,
		    const FFState* prev_state,
		    ScoreComponentCollection* accumulator) const {

    accumulator->PlusEquals( this, -40.0 );
    return NULL;

  }

}
