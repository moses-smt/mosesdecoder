// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
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

#include <limits>
#include <iostream>
#include <memory>
#include <sstream>

#include "moses/FF/FFState.h"
#include "Implementation.h"
#include "ChartState.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Manager.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/StaticData.h"
#include "moses/ChartManager.h"
#include "moses/ChartHypothesis.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
LanguageModelImplementation::LanguageModelImplementation(const std::string &line)
  :LanguageModel(line)
  ,m_nGramOrder(NOT_FOUND)
{
}

void LanguageModelImplementation::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "order") {
    m_nGramOrder = Scan<size_t>(value);
  } else if (key == "path") {
    m_filePath = value;
  } else {
    LanguageModel::SetParameter(key, value);
  }

}

void LanguageModelImplementation::ShiftOrPush(std::vector<const Word*> &contextFactor, const Word &word) const
{
  if (contextFactor.size() < GetNGramOrder()) {
    contextFactor.push_back(&word);
  } else if (GetNGramOrder() > 0) {
    // shift
    for (size_t currNGramOrder = 0 ; currNGramOrder < GetNGramOrder() - 1 ; currNGramOrder++) {
      contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
    }
    contextFactor[GetNGramOrder() - 1] = &word;
  }
}

LMResult LanguageModelImplementation::GetValueGivenState(
  const std::vector<const Word*> &contextFactor,
  FFState &state) const
{
  return GetValueForgotState(contextFactor, state);
}

void LanguageModelImplementation::GetState(
  const std::vector<const Word*> &contextFactor,
  FFState &state) const
{
  GetValueForgotState(contextFactor, state);
}

// Calculate score of a phrase.
void LanguageModelImplementation::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const
{
  fullScore  = 0;
  ngramScore = 0;

  oovCount = 0;

  size_t phraseSize = phrase.GetSize();
  if (!phraseSize) return;

  vector<const Word*> contextFactor;
  contextFactor.reserve(GetNGramOrder());
  std::auto_ptr<FFState> state(NewState((phrase.GetWord(0) == GetSentenceStartWord()) ?
                                        GetBeginSentenceState() : GetNullContextState()));
  size_t currPos = 0;
  while (currPos < phraseSize) {
    const Word &word = phrase.GetWord(currPos);

    if (word.IsNonTerminal()) {
      // do nothing. reset ngram. needed to score target phrases during pt loading in chart decoding
      if (!contextFactor.empty()) {
        // TODO: state operator= ?
        state.reset(NewState(GetNullContextState()));
        contextFactor.clear();
      }
    } else {
      ShiftOrPush(contextFactor, word);
      UTIL_THROW_IF2(contextFactor.size() > GetNGramOrder(),
                     "Can only calculate LM score of phrases up to the n-gram order");

      if (word == GetSentenceStartWord()) {
        // do nothing, don't include prob for <s> unigram
        if (currPos != 0) {
          UTIL_THROW2("Either your data contains <s> in a position other than the first word or your language model is missing <s>.  Did you build your ARPA using IRSTLM and forget to run add-start-end.sh?");
        }
      } else {
        LMResult result = GetValueGivenState(contextFactor, *state);
        fullScore += result.score;
        if (contextFactor.size() == GetNGramOrder())
          ngramScore += result.score;
        if (result.unknown) ++oovCount;
      }
    }

    currPos++;
  }
}

FFState *LanguageModelImplementation::EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const
{
  // In this function, we only compute the LM scores of n-grams that overlap a
  // phrase boundary. Phrase-internal scores are taken directly from the
  // translation option.

  // In the case of unigram language models, there is no overlap, so we don't
  // need to do anything.
  if(GetNGramOrder() <= 1)
    return NULL;

  // Empty phrase added? nothing to be done
  if (hypo.GetCurrTargetLength() == 0)
    return ps ? NewState(ps) : NULL;

  IFVERBOSE(2) {
    hypo.GetManager().GetSentenceStats().StartTimeCalcLM();
  }

  const size_t currEndPos = hypo.GetCurrTargetWordsRange().GetEndPos();
  const size_t startPos = hypo.GetCurrTargetWordsRange().GetStartPos();

  // 1st n-gram
  vector<const Word*> contextFactor(GetNGramOrder());
  size_t index = 0;
  for (int currPos = (int) startPos - (int) GetNGramOrder() + 1 ; currPos <= (int) startPos ; currPos++) {
    if (currPos >= 0)
      contextFactor[index++] = &hypo.GetWord(currPos);
    else {
      contextFactor[index++] = &GetSentenceStartWord();
    }
  }
  FFState *res = NewState(ps);
  float lmScore = ps ? GetValueGivenState(contextFactor, *res).score : GetValueForgotState(contextFactor, *res).score;

  // main loop
  size_t endPos = std::min(startPos + GetNGramOrder() - 2
                           , currEndPos);
  for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++) {
    // shift all args down 1 place
    for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i++)
      contextFactor[i] = contextFactor[i + 1];

    // add last factor
    contextFactor.back() = &hypo.GetWord(currPos);

    lmScore += GetValueGivenState(contextFactor, *res).score;
  }

  // end of sentence
  if (hypo.IsSourceCompleted()) {
    const size_t size = hypo.GetSize();
    contextFactor.back() = &GetSentenceEndWord();

    for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i ++) {
      int currPos = (int)(size - GetNGramOrder() + i + 1);
      if (currPos < 0)
        contextFactor[i] = &GetSentenceStartWord();
      else
        contextFactor[i] = &hypo.GetWord((size_t)currPos);
    }
    lmScore += GetValueForgotState(contextFactor, *res).score;
  } else {
    if (endPos < currEndPos) {
      //need to get the LM state (otherwise the last LM state is fine)
      for (size_t currPos = endPos+1; currPos <= currEndPos; currPos++) {
        for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i++)
          contextFactor[i] = contextFactor[i + 1];
        contextFactor.back() = &hypo.GetWord(currPos);
      }
      GetState(contextFactor, *res);
    }
  }
  if (OOVFeatureEnabled()) {
    vector<float> scores(2);
    scores[0] = lmScore;
    scores[1] = 0;
    out->PlusEquals(this, scores);
  } else {
    out->PlusEquals(this, lmScore);
  }

  IFVERBOSE(2) {
    hypo.GetManager().GetSentenceStats().StopTimeCalcLM();
  }
  return res;
}

FFState* LanguageModelImplementation::EvaluateWhenApplied(const ChartHypothesis& hypo, int featureID, ScoreComponentCollection* out) const
{
  LanguageModelChartState *ret = new LanguageModelChartState(hypo, featureID, GetNGramOrder());
  // data structure for factored context phrase (history and predicted word)
  vector<const Word*> contextFactor;
  contextFactor.reserve(GetNGramOrder());

  // initialize language model context state
  FFState *lmState = NewState( GetNullContextState() );

  // initial language model scores
  float prefixScore = 0.0;    // not yet final for initial words (lack context)
  float finalizedScore = 0.0; // finalized, has sufficient context

  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap();

  // loop over rule
  for (size_t phrasePos = 0, wordPos = 0;
       phrasePos < hypo.GetCurrTargetPhrase().GetSize();
       phrasePos++) {
    // consult rule for either word or non-terminal
    const Word &word = hypo.GetCurrTargetPhrase().GetWord(phrasePos);

    // regular word
    if (!word.IsNonTerminal()) {
      ShiftOrPush(contextFactor, word);

      // beginning of sentence symbol <s>? -> just update state
      if (word == GetSentenceStartWord()) {
        UTIL_THROW_IF2(phrasePos != 0,
                       "Sentence start symbol must be at the beginning of sentence");
        delete lmState;
        lmState = NewState( GetBeginSentenceState() );
      }
      // score a regular word added by the rule
      else {
        updateChartScore( &prefixScore, &finalizedScore, GetValueGivenState(contextFactor, *lmState).score, ++wordPos );
      }
    }

    // non-terminal, add phrase from underlying hypothesis
    else {
      // look up underlying hypothesis
      size_t nonTermIndex = nonTermIndexMap[phrasePos];
      const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);

      const LanguageModelChartState* prevState =
        static_cast<const LanguageModelChartState*>(prevHypo->GetFFState(featureID));

      size_t subPhraseLength = prevState->GetNumTargetTerminals();

      // special case: rule starts with non-terminal -> copy everything
      if (phrasePos == 0) {

        // get prefixScore and finalizedScore
        prefixScore = prevState->GetPrefixScore();
        finalizedScore = -prefixScore;

        // get language model state
        delete lmState;
        lmState = NewState( prevState->GetRightContext() );

        // push suffix
        int suffixPos = prevState->GetSuffix().GetSize() - (GetNGramOrder()-1);
        if (suffixPos < 0) suffixPos = 0; // push all words if less than order
        for(; (size_t)suffixPos < prevState->GetSuffix().GetSize(); suffixPos++) {
          const Word &word = prevState->GetSuffix().GetWord(suffixPos);
          ShiftOrPush(contextFactor, word);
          wordPos++;
        }
      }

      // internal non-terminal
      else {
        // score its prefix
        for(size_t prefixPos = 0;
            prefixPos < GetNGramOrder()-1 // up to LM order window
            && prefixPos < subPhraseLength; // up to length
            prefixPos++) {
          const Word &word = prevState->GetPrefix().GetWord(prefixPos);
          ShiftOrPush(contextFactor, word);
          updateChartScore( &prefixScore, &finalizedScore, GetValueGivenState(contextFactor, *lmState).score, ++wordPos );
        }

        finalizedScore -= prevState->GetPrefixScore();

        // check if we are dealing with a large sub-phrase
        if (subPhraseLength > GetNGramOrder() - 1) {
          // copy language model state
          delete lmState;
          lmState = NewState( prevState->GetRightContext() );

          // push its suffix
          size_t remainingWords = subPhraseLength - (GetNGramOrder()-1);
          if (remainingWords > GetNGramOrder()-1) {
            // only what is needed for the history window
            remainingWords = GetNGramOrder()-1;
          }
          for(size_t suffixPos = prevState->GetSuffix().GetSize() - remainingWords;
              suffixPos < prevState->GetSuffix().GetSize();
              suffixPos++) {
            const Word &word = prevState->GetSuffix().GetWord(suffixPos);
            ShiftOrPush(contextFactor, word);
          }
          wordPos += subPhraseLength;
        }
      }
    }
  }

  // add combined score to score breakdown
  if (OOVFeatureEnabled()) {
    vector<float> scores(2);
    scores[0] = prefixScore + finalizedScore - hypo.GetTranslationOption().GetScores().GetScoresForProducer(this)[0];
    // scores[1] = out->GetScoresForProducer(this)[1];
    scores[1] = 0;
    out->PlusEquals(this, scores);
  } else {
    out->PlusEquals(this, prefixScore + finalizedScore - hypo.GetTranslationOption().GetScores().GetScoresForProducer(this)[0]);
  }

  ret->Set(prefixScore, lmState);
  return ret;
}

void LanguageModelImplementation::updateChartScore(float *prefixScore, float *finalizedScore, float score, size_t wordPos) const
{
  if (wordPos < GetNGramOrder()) {
    *prefixScore += score;
  } else {
    *finalizedScore += score;
  }
}

}
