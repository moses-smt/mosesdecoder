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
#include "moses/LM/Backward.h"
#include "util/exception.hh"

//#include "moses/Util.h"
//#include "moses/StaticData.h"
//#include <iostream>

namespace Moses
{

/** Constructs a new backward language model. */
// TODO(lane): load_method instead of lazy bool
template <class Model> BackwardLanguageModel<Model>::BackwardLanguageModel(const std::string &line, const std::string &file, FactorType factorType, bool lazy) : LanguageModelKen<Model>(line,file,factorType, lazy ? util::LAZY : util::POPULATE_OR_READ)
{
  //
  // This space intentionally left blank
  //
}

/**
 * Constructs an empty backward language model state.
 *
 * This state will correspond with a translation hypothesis
 * where no source words have been translated.
 *
 * In a forward language model, the language model state of an empty hypothesis
 * would store the beginning of sentence marker <s>.
 *
 * Because this is a backward language model, the language model state returned by this method
 * instead stores the end of sentence marker </s>.
 */
template <class Model> const FFState *BackwardLanguageModel<Model>::EmptyHypothesisState(const InputType &/*input*/) const
{
  BackwardLMState *ret = new BackwardLMState();
  lm::ngram::RuleScore<Model> ruleScore(*m_ngram, ret->state);
  ruleScore.Terminal(m_ngram->GetVocabulary().EndSentence());
  //    float score =
  ruleScore.Finish();
  //    VERBOSE(1, "BackwardLM EmptyHypothesisState has score " << score);
  return ret;
}
/*
template <class Model> double BackwardLanguageModel<Model>::Score(FFState *ffState) {
  BackwardLMState *lmState = static_cast< BackwardLMState* >(ffState);
  lm::ngram::ChartState &state = lmState->state;
  lm::ngram::RuleScore<Model> ruleScore(*m_ngram, lmState);
  return ruleScore.Finish();
}
*/
/**
 * Pre-calculate the n-gram probabilities for the words in the specified phrase.
 *
 * Note that when this method is called, we do not have access to the context
 * in which this phrase will eventually be applied.
 *
 * In other words, we know what words are in this phrase,
 * but we do not know what words will come before or after this phrase.
 *
 * The parameters fullScore, ngramScore, and oovCount are all output parameters.
 *
 * The value stored in oovCount is the number of words in the phrase
 * that are not in the language model's vocabulary.
 *
 * The sum of the ngram scores for all words in this phrase are stored in fullScore.
 *
 * The value stored in ngramScore is similar, but only full-order ngram scores are included.
 *
 * This is best shown by example:
 *
 * Assume a trigram backward language model and a phrase "a b c d e f g"
 *
 * fullScore would represent the sum of the logprob scores for the following values:
 *
 * p(g)
 * p(f | g)
 * p(e | g f)
 * p(d | f e)
 * p(c | e d)
 * p(b | d c)
 * p(a | c b)
 *
 * ngramScore would represent the sum of the logprob scores for the following values:
 *
 * p(g)
 * p(f | g)
 * p(e | g f)
 * p(d | f e)
 * p(c | e d)
 * p(b | d c)
 * p(a | c b)
 */
template <class Model> void BackwardLanguageModel<Model>::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const
{
  fullScore = 0;
  ngramScore = 0;
  oovCount = 0;

  if (!phrase.GetSize()) return;

  lm::ngram::ChartState discarded_sadly;
  lm::ngram::RuleScore<Model> scorer(*m_ngram, discarded_sadly);

  UTIL_THROW_IF2(m_beginSentenceFactor == phrase.GetWord(0).GetFactor(m_factorType),
                 "BackwardLanguageModel does not currently support rules that include <s>"
                );

  float before_boundary = 0.0f;

  int lastWord = phrase.GetSize() - 1;
  int ngramBoundary = m_ngram->Order() - 1;
  int boundary = ( lastWord < ngramBoundary ) ? 0 : ngramBoundary;

  int position;
  for (position = lastWord; position >= 0; position-=1) {
    const Word &word = phrase.GetWord(position);
    UTIL_THROW_IF2(word.IsNonTerminal(),
                   "BackwardLanguageModel does not currently support rules that include non-terminals "
                  );

    lm::WordIndex index = TranslateID(word);
    scorer.Terminal(index);
    if (!index) ++oovCount;

    if (position==boundary) {
      before_boundary = scorer.Finish();
    }

  }

  fullScore = scorer.Finish();

  ngramScore = TransformLMScore(fullScore - before_boundary);
  fullScore = TransformLMScore(fullScore);

}

/**
 * Calculate the ngram probabilities for the words at the beginning
 * (and under some circumstances, also at the end)
 * of the phrase represented by the provided hypothesis.
 *
 * Additionally, calculate a new language model state.
 *
 * This is best shown by example:
 *
 * Assume a trigram language model.
 *
 * Assume the previous phrase was "a b c d e f g",
 * which means the previous language model state is "g f".
 *
 * When the phrase corresponding to "a b c d e f g" was previously processed by CalcScore
 * the following full-order ngrams would have been calculated:
 *
 * p(a | c b)
 * p(b | d c)
 * p(c | e d)
 * p(d | f e)
 * p(e | g f)
 *
 * The following less-than-full-order ngrams would also have been calculated by CalcScore:
 *
 * p(f | g)
 * p(g)
 *
 * In this method, we now have access to additional context which may allow
 * us to compute the full-order ngrams for f and g.
 *
 * Assume the new provided hypothesis contains the new phrase "h i j k"
 *
 * Given these assumptions, this method is responsible
 * for calculating the scores for the following:
 *
 * p(f | h g)
 * p(g | i h)
 *
 * This method must also calculate and return a new language model state.
 *
 * In this example, the returned language model state would be "k j"
 *
 * If the provided hypothesis represents the end of a completed translation
 * (all source words have been translated)
 * then this method is additionally responsible for calculating the following:
 *
 * p(j | <s> k)
 * p(k | <s>)
 *
 */
template <class Model> FFState *BackwardLanguageModel<Model>::Evaluate(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const
{

  // If the current hypothesis contains zero target words
  if (!hypo.GetCurrTargetLength()) {

    // reuse and return the previous state
    std::auto_ptr<BackwardLMState> ret(new BackwardLMState());
    ret->state = static_cast<const BackwardLMState&>(*ps).state;
    return ret.release();

  } else {

    float returnedScore;

    FFState *returnedState = this->Evaluate(hypo.GetCurrTargetPhrase(), ps, returnedScore);

    out->PlusEquals(this, returnedScore);

    return returnedState;

  }
}


template <class Model> FFState *BackwardLanguageModel<Model>::Evaluate(const Phrase &phrase, const FFState *ps, float &returnedScore) const
{

  returnedScore = 0.0f;

  const lm::ngram::ChartState &previous = static_cast<const BackwardLMState&>(*ps).state;

  std::auto_ptr<BackwardLMState> ret(new BackwardLMState());

  lm::ngram::RuleScore<Model> scorer(*m_ngram, ret->state);

  int ngramBoundary = m_ngram->Order() - 1;
  int lastWord = phrase.GetSize() - 1;

  // Get scores for words at the end of the previous phrase
  // that are now adjacent to words at the the beginning of this phrase
  for (int position=std::min( lastWord,  ngramBoundary - 1); position >= 0; position-=1) {
    const Word &word = phrase.GetWord(position);
    UTIL_THROW_IF2(word.IsNonTerminal(),
                   "BackwardLanguageModel does not currently support rules that include non-terminals "
                  );

    lm::WordIndex index = TranslateID(word);
    scorer.Terminal(index);
  }
  scorer.NonTerminal(previous);
  returnedScore = scorer.Finish();
  /*
  out->PlusEquals(this, score);


    UTIL_THROW_IF(
      (1==1),
      util::Exception,
      "This method (BackwardLanguageModel<Model>::Evaluate) is not yet fully implemented"
      );
  */
  return ret.release();



}

LanguageModel *ConstructBackwardLM(const std::string &line, const std::string &file, FactorType factorType, bool lazy)
{
  lm::ngram::ModelType model_type;
  if (lm::ngram::RecognizeBinary(file.c_str(), model_type)) {
    switch(model_type) {
    case lm::ngram::PROBING:
      return new BackwardLanguageModel<lm::ngram::ProbingModel>(line, file,  factorType, lazy);
    case lm::ngram::REST_PROBING:
      return new BackwardLanguageModel<lm::ngram::RestProbingModel>(line, file, factorType, lazy);
    case lm::ngram::TRIE:
      return new BackwardLanguageModel<lm::ngram::TrieModel>(line, file, factorType, lazy);
    case lm::ngram::QUANT_TRIE:
      return new BackwardLanguageModel<lm::ngram::QuantTrieModel>(line, file, factorType, lazy);
    case lm::ngram::ARRAY_TRIE:
      return new BackwardLanguageModel<lm::ngram::ArrayTrieModel>(line, file, factorType, lazy);
    case lm::ngram::QUANT_ARRAY_TRIE:
      return new BackwardLanguageModel<lm::ngram::QuantArrayTrieModel>(line, file, factorType, lazy);
    default:
      UTIL_THROW2("Unrecognized kenlm model type " << model_type);
    }
  } else {
    return new BackwardLanguageModel<lm::ngram::ProbingModel>(line, file, factorType, lazy);
  }
}

} // namespace Moses
