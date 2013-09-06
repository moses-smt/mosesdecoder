// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_LanguageModelBackward_h
#define moses_LanguageModelBackward_h

#include <string>

#include "moses/LM/Ken.h"
#include "moses/LM/BackwardLMState.h"

#include "lm/state.hh"

namespace Moses
{

//! This will also load. Returns a templated backward LM.
LanguageModel *ConstructBackwardLM(const std::string &line, const std::string &file, FactorType factorType, bool lazy);

class FFState;
// template<typename M> class BackwardLanguageModelTest;
class BackwardLanguageModelTest;

/*
 * An implementation of single factor backward LM using Kenneth's code.
 */
template <class Model> class BackwardLanguageModel : public LanguageModelKen<Model>
{
public:
  BackwardLanguageModel(const std::string &line, const std::string &file, FactorType factorType, bool lazy);

  virtual const FFState *EmptyHypothesisState(const InputType &/*input*/) const;

  virtual void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const;

  virtual FFState *Evaluate(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const;

  FFState *Evaluate(const Phrase &phrase, const FFState *ps, float &returnedScore) const;

private:

  // These lines are required to make the parent class's protected members visible to this class
  using LanguageModelKen<Model>::m_ngram;
  using LanguageModelKen<Model>::m_beginSentenceFactor;
  using LanguageModelKen<Model>::m_factorType;
  using LanguageModelKen<Model>::TranslateID;

  //    friend class Moses::BackwardLanguageModelTest<Model>;
  friend class Moses::BackwardLanguageModelTest;
  /*
  lm::ngram::ChartState* GetState(FFState *ffState) {
    return NULL;
  }
  */
  /*
  double Score(FFState *ffState) {
  BackwardLMState *lmState = static_cast< BackwardLMState* >(ffState);
  lm::ngram::ChartState &state = lmState->state;
  lm::ngram::RuleScore<Model> ruleScore(*m_ngram, lmState);
  return ruleScore.Finish();
  }
  */
};

} // namespace Moses

#endif

// To create a sample backward language model using SRILM:
//
// (ngram-count and reverse-text are SRILM programs)
//
// head -n 49 ./contrib/synlm/hhmm/LICENSE | tail -n 45 | tr '\n' ' ' | ./scripts/ems/support/split-sentences.perl | ./scripts/tokenizer/lowercase.perl | ./scripts/tokenizer/tokenizer.perl | reverse-text | ngram-count -order 3 -text - -lm - > lm/backward.arpa
