#include "TargetBigramFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"

namespace Moses {

using namespace std;

int TargetBigramState::Compare(const FFState& other) const {
  const TargetBigramState& rhs = dynamic_cast<const TargetBigramState&>(other);
  return Word::Compare(m_word,rhs.m_word);
}

bool TargetBigramFeature::Load(const std::string &filePath) 
{
  if (filePath == "*") return true; //allow all
  ifstream inFile(filePath.c_str());
  if (!inFile)
  {
      return false;
  }

  std::string line;
  m_vocab.insert(BOS_);
  m_vocab.insert(BOS_);
  while (getline(inFile, line)) {
    m_vocab.insert(line);
  }

  inFile.close();
  return true;
}

string TargetBigramFeature::GetScoreProducerWeightShortName(unsigned) const
{
	return "dlmb";
}

size_t TargetBigramFeature::GetNumInputScores() const
{
	return 0;
}


const FFState* TargetBigramFeature::EmptyHypothesisState(const InputType &/*input*/) const
{
  return new TargetBigramState(m_bos);
}

FFState* TargetBigramFeature::Evaluate(const Hypothesis& cur_hypo,
                                       const FFState* prev_state,
                                       ScoreComponentCollection* accumulator) const
{
  const TargetBigramState* tbState = dynamic_cast<const TargetBigramState*>(prev_state);
  CHECK(tbState);

  // current hypothesis target phrase
  const Phrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  if (targetPhrase.GetSize() == 0) {
    return new TargetBigramState(*tbState);
  }

  // extract all bigrams w1 w2 from current hypothesis
  for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
    const Factor* f1 = NULL;
    if (i == 0) {
      f1 = tbState->GetWord().GetFactor(m_factorType);
    } else {
      f1 = targetPhrase.GetWord(i-1).GetFactor(m_factorType);
    }
    const Factor* f2 = targetPhrase.GetWord(i).GetFactor(m_factorType);
    const string& w1 = f1->GetString();
    const string& w2 = f2->GetString();

    // skip bigrams if they don't belong to a given restricted vocabulary
    if (m_vocab.size() && 
        (m_vocab.find(w1) == m_vocab.end() || m_vocab.find(w2) == m_vocab.end())) {
      continue;
    }

    string name(w1 +":"+w2);
    accumulator->PlusEquals(this,name,1);
  }

  if (cur_hypo.GetWordsBitmap().IsComplete()) {
    const string& w1 = targetPhrase.GetWord(targetPhrase.GetSize()-1).GetFactor(m_factorType)->GetString();
    const string& w2 = EOS_;
    if (m_vocab.empty() || (m_vocab.find(w1) != m_vocab.end())) {
      string name(w1 +":"+w2);
      accumulator->PlusEquals(this,name,1);
    }
    return NULL;
  }
  return new TargetBigramState(targetPhrase.GetWord(targetPhrase.GetSize()-1));
}
}

