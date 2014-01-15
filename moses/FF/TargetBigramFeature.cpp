#include "TargetBigramFeature.h"
#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/Hypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "util/string_piece_hash.hh"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

int TargetBigramState::Compare(const FFState& other) const
{
  const TargetBigramState& rhs = dynamic_cast<const TargetBigramState&>(other);
  return Word::Compare(m_word,rhs.m_word);
}

TargetBigramFeature::TargetBigramFeature(const std::string &line)
  :StatefulFeatureFunction(0, line)
{
  std::cerr << "Initializing target bigram feature.." << std::endl;
  ReadParameters();

  FactorCollection& factorCollection = FactorCollection::Instance();
  const Factor* bosFactor =
    factorCollection.AddFactor(Output,m_factorType,BOS_);
  m_bos.SetFactor(m_factorType,bosFactor);

}

void TargetBigramFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else if (key == "path") {
    m_filePath = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

void TargetBigramFeature::Load()
{
  if (m_filePath == "*")
    return ; //allow all
  ifstream inFile(m_filePath.c_str());
  UTIL_THROW_IF2(!inFile, "Can't open file " << m_filePath);

  std::string line;
  m_vocab.insert(BOS_);
  m_vocab.insert(BOS_);
  while (getline(inFile, line)) {
    m_vocab.insert(line);
  }

  inFile.close();
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
  assert(tbState);

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
    const StringPiece w1 = f1->GetString();
    const StringPiece w2 = f2->GetString();

    // skip bigrams if they don't belong to a given restricted vocabulary
    if (m_vocab.size() &&
        (FindStringPiece(m_vocab, w1) == m_vocab.end() || FindStringPiece(m_vocab, w2) == m_vocab.end())) {
      continue;
    }

    string name(w1.data(), w1.size());
    name += ":";
    name.append(w2.data(), w2.size());
    accumulator->PlusEquals(this,name,1);
  }

  if (cur_hypo.GetWordsBitmap().IsComplete()) {
    const StringPiece w1 = targetPhrase.GetWord(targetPhrase.GetSize()-1).GetFactor(m_factorType)->GetString();
    const string& w2 = EOS_;
    if (m_vocab.empty() || (FindStringPiece(m_vocab, w1) != m_vocab.end())) {
      string name(w1.data(), w1.size());
      name += ":";
      name += w2;
      accumulator->PlusEquals(this,name,1);
    }
    return NULL;
  }
  return new TargetBigramState(targetPhrase.GetWord(targetPhrase.GetSize()-1));
}

bool TargetBigramFeature::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[m_factorType];
  return ret;
}

}

