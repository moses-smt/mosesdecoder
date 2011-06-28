#include "PhraseBoundaryFeature.h"

#include "Hypothesis.h"

using namespace std;

namespace Moses {

int PhraseBoundaryState::Compare(const FFState& other) const 
{
  const PhraseBoundaryState& rhs = dynamic_cast<const PhraseBoundaryState&>(other);
  return Word::Compare(*m_word,*(rhs.m_word));
}


PhraseBoundaryFeature::PhraseBoundaryFeature
  (const FactorList& sourceFactors, const FactorList& targetFactors) :
    StatefulFeatureFunction("pb"), m_sourceFactors(sourceFactors),
      m_targetFactors(targetFactors)
{
}

size_t PhraseBoundaryFeature::GetNumScoreComponents() const 
{
  return ScoreProducer::unlimited;
}

string PhraseBoundaryFeature::GetScoreProducerWeightShortName() const 
{
  return "pb";
}

size_t PhraseBoundaryFeature::GetNumInputScores() const 
{
  return 0;
}

const FFState* PhraseBoundaryFeature::EmptyHypothesisState(const InputType &input) const 
{
  return new PhraseBoundaryState(NULL);
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
  const Word* leftWord = pbState->GetWord();
  const Word* rightWord = &(targetPhrase.GetWord(0));
  AddFeatures(leftWord,rightWord,m_sourceFactors,"src",scores);
  AddFeatures(leftWord,rightWord,m_targetFactors,"tgt",scores);

  const Word* endWord = &(targetPhrase.GetWord(targetPhrase.GetSize()-1));

  //if end of sentence add EOS
  if (cur_hypo.IsSourceCompleted()) {
    AddFeatures(endWord,NULL,m_sourceFactors,"src",scores);
    AddFeatures(endWord,NULL,m_targetFactors,"tgt",scores);
  }

  return new PhraseBoundaryState(endWord);
}


}
