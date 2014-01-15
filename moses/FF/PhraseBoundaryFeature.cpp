#include "PhraseBoundaryFeature.h"

#include "moses/Hypothesis.h"
#include "moses/TranslationOption.h"
#include "moses/InputPath.h"

using namespace std;

namespace Moses
{

int PhraseBoundaryState::Compare(const FFState& other) const
{
  const PhraseBoundaryState& rhs = dynamic_cast<const PhraseBoundaryState&>(other);
  int tgt = Word::Compare(*m_targetWord,*(rhs.m_targetWord));
  if (tgt) return tgt;
  return Word::Compare(*m_sourceWord,*(rhs.m_sourceWord));
}

PhraseBoundaryFeature::PhraseBoundaryFeature(const std::string &line)
  : StatefulFeatureFunction(0, line)
{
  std::cerr << "Initializing source word deletion feature.." << std::endl;
  ReadParameters();
}

void PhraseBoundaryFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "source") {
    m_sourceFactors = Tokenize<FactorType>(value, ",");
  } else if (key == "target") {
    m_targetFactors = Tokenize<FactorType>(value, ",");
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

const FFState* PhraseBoundaryFeature::EmptyHypothesisState(const InputType &) const
{
  return new PhraseBoundaryState(NULL,NULL);
}


void PhraseBoundaryFeature::AddFeatures(
  const Word* leftWord, const Word* rightWord, const FactorList& factors, const string& side,
  ScoreComponentCollection* scores) const
{
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

  const Phrase& sourcePhrase = cur_hypo.GetTranslationOption().GetInputPath().GetPhrase();
  const Word* leftSourceWord = pbState->GetSourceWord();
  const Word* rightSourceWord = &(sourcePhrase.GetWord(0));
  AddFeatures(leftSourceWord,rightSourceWord,m_sourceFactors,"src",scores);

  const Word* endSourceWord = &(sourcePhrase.GetWord(sourcePhrase.GetSize()-1));
  const Word* endTargetWord = &(targetPhrase.GetWord(targetPhrase.GetSize()-1));

  //if end of sentence add EOS
  if (cur_hypo.IsSourceCompleted()) {
    AddFeatures(endSourceWord,NULL,m_sourceFactors,"src",scores);
    AddFeatures(endTargetWord,NULL,m_targetFactors,"tgt",scores);
  }

  return new PhraseBoundaryState(endSourceWord,endTargetWord);
}

bool PhraseBoundaryFeature::IsUseable(const FactorMask &mask) const
{
  for (size_t i = 0; i < m_targetFactors.size(); ++i) {
    const FactorType &factor = m_targetFactors[i];
    if (!mask[factor]) {
      return false;
    }
  }
  return true;
}

}
