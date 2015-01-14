#pragma once

#include <string>
#include "lm/model.hh"
#include <boost/shared_ptr.hpp>

namespace Moses
{

class KenOSMBase
{
public:
  virtual float Score(const lm::ngram::State&, const std::string&,
                      lm::ngram::State&) const = 0;

  virtual const lm::ngram::State &BeginSentenceState() const = 0;

  virtual const lm::ngram::State &NullContextState() const = 0;
};

template <class KenModel>
class KenOSM : public KenOSMBase
{
public:
  KenOSM(const std::string& file)
    : m_kenlm(new KenModel(file.c_str())) {}

  virtual float Score(const lm::ngram::State &in_state,
                      const std::string& word,
                      lm::ngram::State &out_state) const {
    return m_kenlm->Score(in_state, m_kenlm->GetVocabulary().Index(word),
                          out_state);
  }

  virtual const lm::ngram::State &BeginSentenceState() const {
    return m_kenlm->BeginSentenceState();
  }

  virtual const lm::ngram::State &NullContextState() const {
    return m_kenlm->NullContextState();
  }

private:
  boost::shared_ptr<KenModel> m_kenlm;
};

typedef KenOSMBase OSMLM;

OSMLM* ConstructOSMLM(const std::string &file);


} // namespace
