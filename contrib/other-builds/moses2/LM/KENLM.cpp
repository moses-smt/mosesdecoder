/*
 * KENLM.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */

#include "KENLM.h"
#include "../System.h"
#include "lm/state.hh"
#include "moses/FactorCollection.h"

struct KenLMState : public Moses::FFState {
  lm::ngram::State state;
  virtual size_t hash() const {
    size_t ret = hash_value(state);
    return ret;
  }
  virtual bool operator==(const Moses::FFState& o) const {
    const KenLMState &other = static_cast<const KenLMState &>(o);
    bool ret = state == other.state;
    return ret;
  }

};

/////////////////////////////////////////////////////////////////
class MappingBuilder : public lm::EnumerateVocab
{
public:
  MappingBuilder(Moses::FactorCollection &factorCollection, std::vector<lm::WordIndex> &mapping)
    : m_factorCollection(factorCollection), m_mapping(mapping) {}

  void Add(lm::WordIndex index, const StringPiece &str) {
    std::size_t factorId = m_factorCollection.AddFactor(str)->GetId();
    if (m_mapping.size() <= factorId) {
      // 0 is <unk> :-)
      m_mapping.resize(factorId + 1);
    }
    m_mapping[factorId] = index;
  }

private:
  Moses::FactorCollection &m_factorCollection;
  std::vector<lm::WordIndex> &m_mapping;
};

/////////////////////////////////////////////////////////////////
KENLM::KENLM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

KENLM::~KENLM()
{
	// TODO Auto-generated destructor stub
}

void KENLM::Load(System &system)
{
	  lm::ngram::Config config;
      config.messages = NULL;

      Moses::FactorCollection &collection = system.GetVocab();
	  MappingBuilder builder(collection, m_lmIdLookup);
	  config.enumerate_vocab = &builder;
	  //config.load_method = lazy ? util::LAZY : util::POPULATE_OR_READ;

	  m_ngram.reset(new Model(m_path.c_str(), config));
}

void KENLM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "factor") {
	  m_factorType = Moses::Scan<Moses::FactorType>(value);
  }
  else if (key == "order") {
	  // don't need to store it
  }
  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

//! return the state associated with the empty hypothesis for a given sentence
const Moses::FFState* KENLM::EmptyHypothesisState(const Manager &mgr, const PhraseImpl &input) const
{
  KenLMState *ret = new KenLMState();
  ret->state = m_ngram->BeginSentenceState();
  return ret;
}

void
KENLM::EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
        Scores &scores,
        Scores *estimatedScore) const
{

}

Moses::FFState* KENLM::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const Moses::FFState &prevState,
  Scores &scores) const
{

 KenLMState *ret = new KenLMState();
  ret->state = m_ngram->BeginSentenceState();
  return ret;
}

