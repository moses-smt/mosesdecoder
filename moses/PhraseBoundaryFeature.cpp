#include "PhraseBoundaryFeature.h"

#include "Hypothesis.h"

using namespace std;

namespace Moses {

int PhraseBoundaryState::Compare(const FFState& other) const 
{
  const PhraseBoundaryState& rhs = dynamic_cast<const PhraseBoundaryState&>(other);
  int tgt = Word::Compare(*m_targetWord,*(rhs.m_targetWord));
  if (tgt) return tgt;
  return Word::Compare(*m_sourceWord,*(rhs.m_sourceWord));
}

PhraseBoundaryFeature::PhraseBoundaryFeature(const std::string &line)
: StatefulFeatureFunction("PhraseBoundaryFeature", FeatureFunction::unlimited, line)
{
  std::cerr << "Initializing source word deletion feature.." << std::endl;

  for (size_t i = 0; i < m_args.size(); ++i) {
    const vector<string> &args = m_args[i];

    if (args[0] == "source") {
      m_sourceFactors = Tokenize<FactorType>(args[1], ",");
    }
    else if (args[0] == "target") {
      m_targetFactors = Tokenize<FactorType>(args[1], ",");
    }
    else {
      throw "Unknown argument " + args[0];
    }
  }
}

const FFState* PhraseBoundaryFeature::EmptyHypothesisState(const InputType &) const 
{
  return new PhraseBoundaryState(NULL,NULL);
}


void PhraseBoundaryFeature::AddFeatures(
  const Word* leftWord, const Word* rightWord, const FactorList& factors, const string& side,
  ScoreComponentCollection* scores) const {
   for (size_t i = 0; i < factors.size(); ++i) {
      ostringstream name;
      name << side << ":";
      name << factors[i];
      name << ":";
      if (leftWord) {
        name << leftWord->GetFactor(factors[i])->GetString();
      } else {
        name << BOS_;
      }
      name << ":";
      if (rightWord) {
        name << rightWord->GetFactor(factors[i])->GetString();
      } else {
        name << EOS_;
      }
      scores->PlusEquals(this,name.str(),1);
    }

}

FFState* PhraseBoundaryFeature::Evaluate
  (const Hypothesis& cur_hypo, const FFState* prev_state,
      ScoreComponentCollection* scores) const
{
  const PhraseBoundaryState* pbState = dynamic_cast<const PhraseBoundaryState*>(prev_state);
  const Phrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  if (targetPhrase.GetSize() == 0) {
    return new PhraseBoundaryState(*pbState);
  }
  const Word* leftTargetWord = pbState->GetTargetWord();
  const Word* rightTargetWord = &(targetPhrase.GetWord(0));
  AddFeatures(leftTargetWord,rightTargetWord,m_targetFactors,"tgt",scores);

  const Phrase* sourcePhrase = cur_hypo.GetSourcePhrase();
  const Word* leftSourceWord = pbState->GetSourceWord();
  const Word* rightSourceWord = &(sourcePhrase->GetWord(0));
  AddFeatures(leftSourceWord,rightSourceWord,m_sourceFactors,"src",scores);

  const Word* endSourceWord = &(sourcePhrase->GetWord(sourcePhrase->GetSize()-1));
  const Word* endTargetWord = &(targetPhrase.GetWord(targetPhrase.GetSize()-1));

  //if end of sentence add EOS
  if (cur_hypo.IsSourceCompleted()) {
    AddFeatures(endSourceWord,NULL,m_sourceFactors,"src",scores);
    AddFeatures(endTargetWord,NULL,m_targetFactors,"tgt",scores);
  }

  return new PhraseBoundaryState(endSourceWord,endTargetWord);
}

void PhraseBoundaryFeature::Evaluate(const TargetPhrase &targetPhrase
                      , ScoreComponentCollection &scoreBreakdown
                      , float &estimatedFutureScore) const
{
  CHECK(false);
}


}
