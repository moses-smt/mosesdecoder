#pragma once

#include <string>
#include "lm/model.hh"

namespace Moses
{

class KenDsgBase
{
public:
  virtual ~KenDsgBase() {}

  virtual float Score(const lm::ngram::State&, StringPiece,
                      lm::ngram::State&) const = 0;

  virtual const lm::ngram::State &BeginSentenceState() const = 0;

  virtual const lm::ngram::State &NullContextState() const = 0;

  virtual float ScoreEndSentence(const lm::ngram::State &in_state, lm::ngram::State &out_state) const = 0;
};

template <class KenModel>
class KenDsg : public KenDsgBase
{
public:
  KenDsg(const char *file, const lm::ngram::Config &config)
    : m_kenlm(file, config) {}

  float Score(const lm::ngram::State &in_state,
              StringPiece word,
              lm::ngram::State &out_state) const {
    return m_kenlm.Score(in_state, m_kenlm.GetVocabulary().Index(word),
                         out_state);
  }

  const lm::ngram::State &BeginSentenceState() const {
    return m_kenlm.BeginSentenceState();
  }

  const lm::ngram::State &NullContextState() const {
    return m_kenlm.NullContextState();
  }

  float ScoreEndSentence(const lm::ngram::State &in_state, lm::ngram::State &out_state) const {
    return m_kenlm.Score(in_state, m_kenlm.GetVocabulary().EndSentence(), out_state);
  }


private:
  KenModel m_kenlm;
};

typedef KenDsgBase DsgLM;

DsgLM* ConstructDsgLM(const char *file);


} // namespace
