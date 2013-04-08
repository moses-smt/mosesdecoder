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

#include "moses/FFState.h"
#include "moses/Phrase.h"

#include "moses/LM/Ken.h"
#include "moses/LM/Backward.h"

namespace Moses {
  
  // By placing BackwardLMState inside an anonymous namespace,
  // it is visible *only* within this file
  namespace {

    struct BackwardLMState : public FFState {
      lm::ngram::ChartState state;
      int Compare(const FFState &o) const {
	const BackwardLMState &other = static_cast<const BackwardLMState &>(o);
 	return state.left.Compare(other.state.left);
      }
    };

  }

  template <class Model> BackwardLanguageModel<Model>::BackwardLanguageModel(const std::string &file, FactorType factorType, bool lazy) : LanguageModelKen<Model>(file,factorType,lazy) {
    //
    // This space intentionally left blank
    //
  }

  template <class Model> const FFState *BackwardLanguageModel<Model>::EmptyHypothesisState(const InputType &/*input*/) const {
    BackwardLMState *ret = new BackwardLMState();
    lm::ngram::RuleScore<Model> ruleScore(*m_ngram, ret->state);
    ruleScore.Terminal(m_ngram->GetVocabulary().EndSentence());
    ruleScore.Finish();
    return ret;
  }

  template <class Model> void BackwardLanguageModel<Model>::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const {
    fullScore = 0;
    ngramScore = 0;
    oovCount = 0;
    
    if (!phrase.GetSize()) return;

    lm::ngram::ChartState discarded_sadly;
    lm::ngram::RuleScore<Model> scorer(*m_ngram, discarded_sadly);
    
    UTIL_THROW_IF(
		  (m_beginSentenceFactor == phrase.GetWord(0).GetFactor(m_factorType)),
		  util::Exception,
		  "BackwardLanguageModel does not currently support rules that include <s>"
		  );
  
    float before_boundary = 0.0f;
    for (size_t position = phrase.GetSize() - 1,
	   ngramBoundary = m_ngram->Order() - 1; position >= 0; position-=1) {
	   
	   const Word &word = phrase.GetWord(position);

	   UTIL_THROW_IF(
			 (word.IsNonTerminal()),
			 util::Exception,
			 "BackwardLanguageModel does not currently support rules that include non-terminals"
			 );
  
	   lm::WordIndex index = TranslateID(word);
	   scorer.Terminal(index);
	   if (!index) ++oovCount;

	   if (position==ngramBoundary) {
	     before_boundary = scorer.Finish();
	   }

    }

    fullScore += scorer.Finish();
    
    ngramScore = TransformLMScore(fullScore - before_boundary);
    fullScore = TransformLMScore(fullScore);

  }

  LanguageModel *ConstructBackwardLM(const std::string &file, FactorType factorType, bool lazy) {
    try {
      lm::ngram::ModelType model_type;
      if (lm::ngram::RecognizeBinary(file.c_str(), model_type)) {
	switch(model_type) {
        case lm::ngram::PROBING:
          return new BackwardLanguageModel<lm::ngram::ProbingModel>(file,  factorType, lazy);
        case lm::ngram::REST_PROBING:
          return new BackwardLanguageModel<lm::ngram::RestProbingModel>(file, factorType, lazy);
        case lm::ngram::TRIE:
          return new BackwardLanguageModel<lm::ngram::TrieModel>(file, factorType, lazy);
        case lm::ngram::QUANT_TRIE:
          return new BackwardLanguageModel<lm::ngram::QuantTrieModel>(file, factorType, lazy);
        case lm::ngram::ARRAY_TRIE:
          return new BackwardLanguageModel<lm::ngram::ArrayTrieModel>(file, factorType, lazy);
        case lm::ngram::QUANT_ARRAY_TRIE:
          return new BackwardLanguageModel<lm::ngram::QuantArrayTrieModel>(file, factorType, lazy);
        default:
          std::cerr << "Unrecognized kenlm model type " << model_type << std::endl;
          abort();
	}
      } else {
	return new BackwardLanguageModel<lm::ngram::ProbingModel>(file, factorType, lazy);
      }
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      abort();
    }
  }

} // namespace Moses
