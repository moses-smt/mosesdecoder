#pragma once

#include <string>
#include "lm/model.hh"

namespace Moses2
{

class KenOSMBase
{
public:
  virtual ~KenOSMBase() {}

  virtual float Score(const lm::ngram::State&, StringPiece,
                      lm::ngram::State&) const = 0;

  virtual const lm::ngram::State &BeginSentenceState() const = 0;

  virtual const lm::ngram::State &NullContextState() const = 0;
};

template <class KenModel>
class KenOSM : public KenOSMBase
{
public:
  KenOSM(const char *file, const lm::ngram::Config &config)
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

private:
  KenModel m_kenlm;
};

typedef KenOSMBase OSMLM;

OSMLM* ConstructOSMLM(const char *file, util::LoadMethod load_method);


} // namespace
