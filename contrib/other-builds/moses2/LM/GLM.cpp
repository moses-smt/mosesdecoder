/*
 * GLM.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <sstream>
#include <vector>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "GLM.h"
#include "../Phrase.h"
#include "../Scores.h"
#include "../System.h"
#include "../PhraseBased/Hypothesis.h"
#include "../PhraseBased/Manager.h"
#include "lm/state.hh"
#include "lm/left.hh"
#include "util/exception.hh"
#include "util/tokenize_piece.hh"
#include "util/string_stream.hh"
#include "../legacy/FactorCollection.h"

using namespace std;

namespace Moses2
{

struct GLMState: public FFState
{
  virtual std::string ToString() const
  {
    return "GLMState";
  }

  virtual size_t hash() const
  {

    return 3434;
  }

  virtual bool operator==(const FFState& other) const
  {

    return true;
  }

  size_t numWords;
  const Factor** lastWords;

};


/////////////////////////////////////////////////////////////////
GLM::GLM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
  cerr << "GLM::GLM" << endl;
  ReadParameters();
}

GLM::~GLM()
{
  // TODO Auto-generated destructor stub
}

void GLM::Load(System &system)
{
  cerr << "GLM::Load" << endl;
  FactorCollection &fc = system.GetVocab();

  m_bos = fc.AddFactor(BOS_, system, false);
  m_eos = fc.AddFactor(EOS_, system, false);

  FactorCollection &collection = system.GetVocab();
}

FFState* GLM::BlankState(MemPool &pool) const
{
  GLMState *ret = new (pool.Allocate<GLMState>()) GLMState();
  return ret;
}

//! return the state associated with the empty hypothesis for a given sentence
void GLM::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
    const InputType &input, const Hypothesis &hypo) const
{
  GLMState &stateCast = static_cast<GLMState&>(state);
  //stateCast.state = m_ngram->BeginSentenceState();
}

void GLM::EvaluateInIsolation(MemPool &pool, const System &system,
    const Phrase<Moses2::Word> &source, const TargetPhrase<Moses2::Word> &targetPhrase, Scores &scores,
    SCORE *estimatedScore) const
{
  if (targetPhrase.GetSize() == 0) {
    return;
  }

  SCORE score = 0;
  SCORE nonFullScore = 0;
  vector<const Factor*> context;
//  context.push_back(m_bos);

  context.reserve(m_order);

}

void GLM::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE *estimatedScore) const
{
  UTIL_THROW2("Not implemented");
}

void GLM::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

void GLM::SetParameter(const std::string& key,
    const std::string& value)
{
  //cerr << "key=" << key << " " << value << endl;
  if (key == "path") {
    m_path = value;
  }
  else if (key == "order") {
    m_order = Scan<size_t>(value);
  }
  else if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  }
  else if (key == "lazyken") {
    m_load_method =
           boost::lexical_cast<bool>(value) ?
           util::LAZY : util::POPULATE_OR_READ;
  }
  else if (key == "load") {
    if (value == "lazy") {
      m_load_method = util::LAZY;
    }
    else if (value == "populate_or_lazy") {
      m_load_method = util::POPULATE_OR_LAZY;
    }
    else if (value == "populate_or_read" || value == "populate") {
      m_load_method = util::POPULATE_OR_READ;
    }
    else if (value == "read") {
      m_load_method = util::READ;
    }
    else if (value == "parallel_read") {
      m_load_method = util::PARALLEL_READ;
    }
    else {
      UTIL_THROW2("Unknown GLM load method " << value);
    }
  }
  else {
    StatefulFeatureFunction::SetParameter(key, value);
  }

  //cerr << "SetParameter done" << endl;
}

void GLM::EvaluateWhenAppliedBatch(
    const Batch &batch) const
{
  for (size_t i = 0; i < batch.size(); ++i) {
    Hypothesis *hypo = batch[i];
    CreateNGram(*hypo);
  }
}

void GLM::CreateNGram(const Hypothesis &hypo) const
{

}

void GLM::ShiftOrPush(std::vector<const Factor*> &context,
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

}

