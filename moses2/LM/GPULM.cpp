/*
 * GPULM.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <sstream>
#include <vector>

#ifdef _linux
#include <pthread.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "GPULM.h"
#include "../Phrase.h"
#include "../Scores.h"
#include "../System.h"
#include "../PhraseBased/Hypothesis.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/TargetPhraseImpl.h"
#include "util/exception.hh"
#include "../legacy/FactorCollection.h"

using namespace std;

namespace Moses2
{

struct GPULMState: public FFState {
  virtual std::string ToString() const {
    return "GPULMState";
  }

  virtual size_t hash() const {
    return boost::hash_value(lastWords);
  }

  virtual bool operator==(const FFState& other) const {
    const GPULMState &otherCast = static_cast<const GPULMState&>(other);
    bool ret = lastWords == otherCast.lastWords;

    return ret;
  }

  void SetContext(const Context &context) {
    lastWords = context;
    if (lastWords.size()) {
      lastWords.resize(lastWords.size() - 1);
    }
  }

  Context lastWords;
};


/////////////////////////////////////////////////////////////////
GPULM::GPULM(size_t startInd, const std::string &line)
  :StatefulFeatureFunction(startInd, line)
{
  cerr << "GPULM::GPULM" << endl;
  ReadParameters();
}

GPULM::~GPULM()
{
  // TODO Auto-generated destructor stub
}

void GPULM::Load(System &system)
{
  cerr << "GPULM::Load" << endl;
  FactorCollection &fc = system.GetVocab();

  m_bos = fc.AddFactor(BOS_, system, false);
  m_eos = fc.AddFactor(EOS_, system, false);

  FactorCollection &collection = system.GetVocab();
}

FFState* GPULM::BlankState(MemPool &pool, const System &sys) const
{
  GPULMState *ret = new (pool.Allocate<GPULMState>()) GPULMState();
  return ret;
}

//! return the state associated with the empty hypothesis for a given sentence
void GPULM::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
                                 const InputType &input, const Hypothesis &hypo) const
{
  GPULMState &stateCast = static_cast<GPULMState&>(state);
  stateCast.lastWords.push_back(m_bos);
}

void GPULM::EvaluateInIsolation(MemPool &pool, const System &system,
                                const Phrase<Moses2::Word> &source, const TargetPhraseImpl &targetPhrase, Scores &scores,
                                SCORE &estimatedScore) const
{
  if (targetPhrase.GetSize() == 0) {
    return;
  }

  SCORE score = 0;
  SCORE nonFullScore = 0;
  Context context;
//  context.push_back(m_bos);

  context.reserve(m_order);
  for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
    const Factor *factor = targetPhrase[i][m_factorType];
    ShiftOrPush(context, factor);

    if (context.size() == m_order) {
      //std::pair<SCORE, void*> fromScoring = Score(context);
      //score += fromScoring.first;
    } else {
      //std::pair<SCORE, void*> fromScoring = Score(context);
      //nonFullScore += fromScoring.first;
    }
  }

}

void GPULM::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                                const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                                SCORE &estimatedScore) const
{
  UTIL_THROW2("Not implemented");
}

void GPULM::EvaluateWhenApplied(const ManagerBase &mgr,
                                const Hypothesis &hypo, const FFState &prevState, Scores &scores,
                                FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

void GPULM::SetParameter(const std::string& key,
                         const std::string& value)
{
  //cerr << "key=" << key << " " << value << endl;
  if (key == "path") {
    m_path = value;
  } else if (key == "order") {
    m_order = Scan<size_t>(value);
  } else if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }

  //cerr << "SetParameter done" << endl;
}

void GPULM::EvaluateWhenAppliedBatch(
  const System &system,
  const Batch &batch) const
{
  // create list of ngrams
  std::vector<std::pair<Hypothesis*, Context> > contexts;

  for (size_t i = 0; i < batch.size(); ++i) {
    Hypothesis *hypo = batch[i];
    CreateNGram(contexts, *hypo);
  }

  // score ngrams
  for (size_t i = 0; i < contexts.size(); ++i) {
    const Context &context = contexts[i].second;
    Hypothesis *hypo = contexts[i].first;
    SCORE score = Score(context);
    Scores &scores = hypo->GetScores();
    scores.PlusEquals(system, *this, score);
  }


}

void GPULM::CreateNGram(std::vector<std::pair<Hypothesis*, Context> > &contexts, Hypothesis &hypo) const
{
  const TargetPhrase<Moses2::Word> &tp = hypo.GetTargetPhrase();

  if (tp.GetSize() == 0) {
    return;
  }

  const Hypothesis *prevHypo = hypo.GetPrevHypo();
  assert(prevHypo);
  const FFState *prevState = prevHypo->GetState(GetStatefulInd());
  assert(prevState);
  const GPULMState &prevStateCast = static_cast<const GPULMState&>(*prevState);

  Context context = prevStateCast.lastWords;
  context.reserve(m_order);

  for (size_t i = 0; i < tp.GetSize(); ++i) {
    const Word &word = tp[i];
    const Factor *factor = word[m_factorType];
    ShiftOrPush(context, factor);

    std::pair<Hypothesis*, Context> ele(&hypo, context);
    contexts.push_back(ele);
  }

  FFState *state = hypo.GetState(GetStatefulInd());
  GPULMState &stateCast = static_cast<GPULMState&>(*state);
  stateCast.SetContext(context);
}

void GPULM::ShiftOrPush(std::vector<const Factor*> &context,
                        const Factor *factor) const
{
  if (context.size() < m_order) {
    context.resize(context.size() + 1);
  }
  assert(context.size());

  for (size_t i = context.size() - 1; i > 0; --i) {
    context[i] = context[i - 1];
  }

  context[0] = factor;
}

SCORE GPULM::Score(const Context &context) const
{
  return 444;
}

void GPULM::EvaluateWhenApplied(const SCFG::Manager &mgr,
                                const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
                                FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

}

