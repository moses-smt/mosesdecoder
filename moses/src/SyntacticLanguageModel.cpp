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
  , m_files(new SyntacticLanguageModelFiles<YModel,XModel>(filePath))
  , m_factorType(factorType)
  , m_beamWidth(beamWidth) {

    // Inform Moses score manager of this feature and its weight(s)
    const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
    const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
    VERBOSE(3,"Constructed SyntacticLanguageModel" << endl);
  }

  SyntacticLanguageModel::~SyntacticLanguageModel() {
    VERBOSE(3,"Destructing SyntacticLanguageModel" << std::endl);
    delete m_files;
  }

  size_t SyntacticLanguageModel::GetNumScoreComponents() const {
    return m_NumScoreComponents;
  }

  std::string SyntacticLanguageModel::GetScoreProducerDescription(unsigned) const {
    return "Syntactic Language Model";
  }

  std::string SyntacticLanguageModel::GetScoreProducerWeightShortName(unsigned) const {
    return "slm";
  }

  const FFState* SyntacticLanguageModel::EmptyHypothesisState(const InputType &input) const {

    return new SyntacticLanguageModelState<YModel,XModel,S,R>(m_files,m_beamWidth);

  }

  /*
  double SyntacticLanguageModel::perplexity() {

    SyntacticLanguageModelState<YModel,XModel,S,R> *prev = 
      new SyntacticLanguageModelState<YModel,XModel,S,R>(m_files,m_beamWidth);

    std::cerr << "Initial prob:" << "\t" << prev->getProb() <<std::endl;


    std::vector<std::string> words(3);
    words[0] = "no";
    words[1] = ",";
    words[2] = "zxvth";


    for (std::vector<std::string>::iterator i=words.begin();
	 i != words.end();
	 i++) {

      prev = new SyntacticLanguageModelState<YModel,XModel,S,R>(prev, *i);
      std::cerr << *i << "\t" << prev->getProb() <<std::endl;

    }

    if (true) exit(-1);

    return prev->getProb();

  }
  */
  FFState* SyntacticLanguageModel::Evaluate(const Hypothesis& cur_hypo,
		    const FFState* prev_state,
		    ScoreComponentCollection* accumulator) const {

    VERBOSE(3,"Evaluating SyntacticLanguageModel for a hypothesis" << endl);

    SyntacticLanguageModelState<YModel,XModel,S,R>*  tmpState = NULL;
    SyntacticLanguageModelState<YModel,XModel,S,R>* nextState = NULL;
  

    const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();

    for (size_t i=0, n=targetPhrase.GetSize(); i<n; i++) {
      
      const Word& word = targetPhrase.GetWord(i);
      const Factor* factor = word.GetFactor(m_factorType);
      
      const std::string& string = factor->GetString();
      
      if (i==0) {
	nextState = new SyntacticLanguageModelState<YModel,XModel,S,R>((const SyntacticLanguageModelState<YModel,XModel,S,R>*)prev_state, string);
      } else {
	tmpState = nextState;
	nextState = new SyntacticLanguageModelState<YModel,XModel,S,R>(tmpState, string);
	delete tmpState;
      }
      
      double score = nextState->getScore();
      VERBOSE(3,"SynLM evaluated a score of " << score << endl);
      accumulator->Assign( this, score );
    }

  

    return nextState;

  }

}
