//

#include "StaticData.h"
#include "SyntacticLanguageModel.h"
#include "HHMMLangModel-gf.h"
#include "TextObsModel.h"
#include "SyntacticLanguageModelFiles.h"
#include "SyntacticLanguageModelState.h"


namespace Moses
{
  //  asnteousntaoheisnthaoesntih
  SyntacticLanguageModel::SyntacticLanguageModel(const std::vector<std::string>& filePath,
						 const std::vector<float>& weights,
						 const FactorType factorType,
						 size_t beamWidth) 
    // Initialize member variables  
  : m_NumScoreComponents(weights.size())
  , m_beamWidth(beamWidth)
  , m_factorType(factorType)
  , m_files(new SyntacticLanguageModelFiles<YModel,XModel>(filePath)) {

    // Inform Moses score manager of this feature and its weight(s)
    const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
    const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);

  }

  SyntacticLanguageModel::~SyntacticLanguageModel() {
    cerr << "Destructing SyntacticLanguageModel" << std::endl;
    //    delete m_files;
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

    return new SyntacticLanguageModelState<YModel,XModel,S,R>(m_files,m_beamWidth);

  }


  FFState* SyntacticLanguageModel::Evaluate(const Hypothesis& cur_hypo,
		    const FFState* prev_state,
		    ScoreComponentCollection* accumulator) const {

    const SyntacticLanguageModelState<YModel,XModel,S,R>& prev =
      static_cast<const SyntacticLanguageModelState<YModel,XModel,S,R>&>(*prev_state);

    const SyntacticLanguageModelState<YModel,XModel,S,R>* currentState = &prev;
    SyntacticLanguageModelState<YModel,XModel,S,R>* nextState = NULL;
  

    const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();

    for (size_t i=0, n=targetPhrase.GetSize(); i<n; i++) {
      
      const Word& word = targetPhrase.GetWord(i);
      const Factor* factor = word.GetFactor(m_factorType);
      
      const std::string& string = factor->GetString();
      
      if (i==0) {
	nextState = new SyntacticLanguageModelState<YModel,XModel,S,R>(&prev, string);
      } else {
	currentState = nextState;
	nextState = new SyntacticLanguageModelState<YModel,XModel,S,R>(currentState, string);
      }
      
      double score = nextState->getScore();

      accumulator->Assign( this, score );
    }

  

    return nextState;

  }

}
