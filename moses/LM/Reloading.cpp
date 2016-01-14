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

#include "lm/binary_format.hh"
#include "lm/enumerate_vocab.hh"
#include "lm/left.hh"
#include "lm/model.hh"

#include "moses/FF/FFState.h"
#include "moses/Hypothesis.h"
#include "moses/Phrase.h"

#include "moses/LM/Ken.h"
#include "moses/LM/Reloading.h"
#include "util/exception.hh"

//#include "moses/Util.h"
//#include "moses/StaticData.h"
//#include <iostream>
/*
namespace Moses
{
namespace
{

struct ReloadingLMState : public FFState {
  lm::ngram::State state;
  virtual size_t hash() const {
    return 0;
  }
  virtual bool operator==(const FFState& o) const {
    return true;
  }

};
} // namespace


template <class Model> ReloadingLanguageModel<Model>::ReloadingLanguageModel(const std::string &line, const std::string &file, FactorType factorType, bool lazy) : LanguageModelKen<Model>(line,file,factorType,lazy)
{
  //
  // This space intentionally left blank
  //
}
template <class Model> const FFState *ReloadingLanguageModel<Model>::EmptyHypothesisState(const InputType &input) const
{
  ReloadingLMState *ret = new ReloadingLMState();
  ret->state = m_ngram->BeginSentenceState();
  return ret;
}


template <class Model> FFState *ReloadingLanguageModel<Model>::EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const
{

  std::auto_ptr<FFState> kenlmState(LanguageModelKen<Model>::EvaluateWhenApplied(hypo, ps, out));
  const lm::ngram::State &out_state = static_cast<const ReloadingLMState&>(*kenlmState).state;


  std::auto_ptr<ReloadingLMState> ret(new ReloadingLMState());
  ret->state = out_state;

  kenlmState.release();
  return ret.release();
}


LanguageModel *ConstructReloadingLM(const std::string &line, const std::string &file, FactorType factorType, bool lazy)
{
  lm::ngram::ModelType model_type;
  if (lm::ngram::RecognizeBinary(file.c_str(), model_type)) {
    switch(model_type) {
    case lm::ngram::PROBING:
      return new ReloadingLanguageModel<lm::ngram::ProbingModel>(line, file,  factorType, lazy);
    case lm::ngram::REST_PROBING:
      return new ReloadingLanguageModel<lm::ngram::RestProbingModel>(line, file, factorType, lazy);
    case lm::ngram::TRIE:
      return new ReloadingLanguageModel<lm::ngram::TrieModel>(line, file, factorType, lazy);
    case lm::ngram::QUANT_TRIE:
      return new ReloadingLanguageModel<lm::ngram::QuantTrieModel>(line, file, factorType, lazy);
    case lm::ngram::ARRAY_TRIE:
      return new ReloadingLanguageModel<lm::ngram::ArrayTrieModel>(line, file, factorType, lazy);
    case lm::ngram::QUANT_ARRAY_TRIE:
      return new ReloadingLanguageModel<lm::ngram::QuantArrayTrieModel>(line, file, factorType, lazy);
    default:
      UTIL_THROW2("Unrecognized kenlm model type " << model_type);
    }
  } else {
    return new ReloadingLanguageModel<lm::ngram::ProbingModel>(line, file, factorType, lazy);
  }
}

} // namespace Moses
*/
