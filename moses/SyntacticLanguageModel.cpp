// This file should be compiled only when the HAVE_SYNLM flag is enabled.
//
// The following ifdef prevents XCode and other non-bjam build systems
// from attempting to compile this file when HAVE_SYNLM is disabled.
//
#ifdef HAVE_SYNLM

//

#include "StaticData.h"
#include "SyntacticLanguageModel.h"
#include "HHMMLangModel-gf.h"
#include "TextObsModel.h"
#include "SyntacticLanguageModelFiles.h"
#include "SyntacticLanguageModelState.h"


namespace Moses
{
SyntacticLanguageModel::SyntacticLanguageModel(const std::string &line)
// Initialize member variables
/*
: m_NumScoreComponents(weights.size())
, m_files(new SyntacticLanguageModelFiles<YModel,XModel>(filePath))
, m_factorType(factorType)
, m_beamWidth(beamWidth) {
*/
{
  /* taken from StaticData::LoadSyntacticLanguageModel()
        cerr << "Loading syntactic language models..." << std::endl;

  const vector<float> weights = Scan<float>(m_parameter->GetParam("weight-slm"));
  const vector<string> files = m_parameter->GetParam("slmodel-file");

  const FactorType factorType = (m_parameter->GetParam("slmodel-factor").size() > 0) ?
    TransformScore(Scan<int>(m_parameter->GetParam("slmodel-factor")[0]))
    : 0;

  const size_t beamWidth = (m_parameter->GetParam("slmodel-beam").size() > 0) ?
    TransformScore(Scan<int>(m_parameter->GetParam("slmodel-beam")[0]))
    : 500;

  if (files.size() < 1) {
    cerr << "No syntactic language model files specified!" << std::endl;
    return false;
  }

  // check if feature is used
  if (weights.size() >= 1) {

    //cout.setf(ios::scientific,ios::floatfield);
    //cerr.setf(ios::scientific,ios::floatfield);

    // create the feature
    m_syntacticLanguageModel = new SyntacticLanguageModel(files,weights,factorType,beamWidth);


    /////////////////////////////////////////
    // BEGIN LANE's UNSTABLE EXPERIMENT :)
    //

    //double ppl = m_syntacticLanguageModel->perplexity();
    //cerr << "Probability is " << ppl << endl;


    //
    // END LANE's UNSTABLE EXPERIMENT
    /////////////////////////////////////////



    if (m_syntacticLanguageModel==NULL) {
      return false;
    }

  }

  return true;

   */
}

SyntacticLanguageModel::~SyntacticLanguageModel()
{
  VERBOSE(3,"Destructing SyntacticLanguageModel" << std::endl);
  delete m_files;
}

size_t SyntacticLanguageModel::GetNumScoreComponents() const
{
  return m_NumScoreComponents;
}

std::string SyntacticLanguageModel::GetScoreProducerDescription() const
{
  return "SyntacticLM";
}

const FFState* SyntacticLanguageModel::EmptyHypothesisState(const InputType &input) const
{

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
    ScoreComponentCollection* accumulator) const
{

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

#endif
