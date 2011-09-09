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

#include <cassert>
#include <limits>
#include <iostream>
#include <memory>
#include <sstream>

#include "FFState.h"
#include "LanguageModelImplementation.h"
#include "TypeDef.h"
#include "Util.h"
#include "Manager.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"
#include "LanguageModelChartState.h"
#include "ChartHypothesis.h"

using namespace std;

namespace Moses
{

void LanguageModelImplementation::ShiftOrPush(std::vector<const Word*> &contextFactor, const Word &word) const
{
  if (contextFactor.size() < GetNGramOrder()) {
    contextFactor.push_back(&word);
  } else {
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

FFState* LanguageModelImplementation::EvaluateChart(const ChartHypothesis& hypo, int featureID, ScoreComponentCollection* out, const LanguageModel *scorer) const {
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
    hypo.GetCurrTargetPhrase().GetAlignmentInfo().GetNonTermIndexMap();

  // loop over rule
  for (size_t phrasePos = 0, wordPos = 0;
       phrasePos < hypo.GetCurrTargetPhrase().GetSize();
       phrasePos++)
  {
    // consult rule for either word or non-terminal
    const Word &word = hypo.GetCurrTargetPhrase().GetWord(phrasePos);

    // regular word
    if (!word.IsNonTerminal())
    {
      ShiftOrPush(contextFactor, word);

      // beginning of sentence symbol <s>? -> just update state
      if (word == GetSentenceStartArray())
      {
        assert(phrasePos == 0);
        delete lmState;
        lmState = NewState( GetBeginSentenceState() );
      }
      // score a regular word added by the rule
      else
      {
        updateChartScore( &prefixScore, &finalizedScore, UntransformLMScore(GetValueGivenState(contextFactor, *lmState).score), ++wordPos );
      }
    }

    // non-terminal, add phrase from underlying hypothesis
    else
    {
      // look up underlying hypothesis
      size_t nonTermIndex = nonTermIndexMap[phrasePos];
      const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);
      size_t subPhraseLength = prevHypo->GetNumTargetTerminals();

      // special case: rule starts with non-terminal -> copy everything
      if (phrasePos == 0) {

        // get prefixScore and finalizedScore
        const LanguageModelChartState* prevState =
          dynamic_cast<const LanguageModelChartState*>(prevHypo->GetFFState( featureID ));
        prefixScore = prevState->GetPrefixScore();
        finalizedScore = prevHypo->GetScoreBreakdown().GetScoresForProducer(scorer)[0] - prefixScore;

        // get language model state
        delete lmState;
        lmState = NewState( prevState->GetRightContext() );

        // push suffix
        int suffixPos = prevHypo->GetSuffix().GetSize() - (GetNGramOrder()-1);
        if (suffixPos < 0) suffixPos = 0; // push all words if less than order
        for(;(size_t)suffixPos < prevHypo->GetSuffix().GetSize(); suffixPos++)
        {
          const Word &word = prevHypo->GetSuffix().GetWord(suffixPos);
          ShiftOrPush(contextFactor, word);
          wordPos++;
        }
      }

      // internal non-terminal
      else
      {
        // score its prefix
        for(size_t prefixPos = 0;
            prefixPos < GetNGramOrder()-1 // up to LM order window
              && prefixPos < subPhraseLength; // up to length
            prefixPos++)
        {
          const Word &word = prevHypo->GetPrefix().GetWord(prefixPos);
          ShiftOrPush(contextFactor, word);
          updateChartScore( &prefixScore, &finalizedScore, UntransformLMScore(GetValueGivenState(contextFactor, *lmState).score), ++wordPos );
        }

        // check if we are dealing with a large sub-phrase
        if (subPhraseLength > GetNGramOrder() - 1)
        {
          // add its finalized language model score
          const LanguageModelChartState* prevState =
            dynamic_cast<const LanguageModelChartState*>(prevHypo->GetFFState( featureID ));
          finalizedScore +=
            prevHypo->GetScoreBreakdown().GetScoresForProducer(scorer)[0] // full score
            - prevState->GetPrefixScore();                              // - prefix score

          // copy language model state
          delete lmState;
          lmState = NewState( prevState->GetRightContext() );

          // push its suffix
          size_t remainingWords = subPhraseLength - (GetNGramOrder()-1);
          if (remainingWords > GetNGramOrder()-1) {
            // only what is needed for the history window
            remainingWords = GetNGramOrder()-1;
          }
          for(size_t suffixPos = prevHypo->GetSuffix().GetSize() - remainingWords;
              suffixPos < prevHypo->GetSuffix().GetSize();
              suffixPos++) {
            const Word &word = prevHypo->GetSuffix().GetWord(suffixPos);
            ShiftOrPush(contextFactor, word);
          }
          wordPos += subPhraseLength;
        }
      }
    }
  }

  // assign combined score to score breakdown
  out->Assign(scorer, prefixScore + finalizedScore);

  // create and return feature function state
  LanguageModelChartState *res = new LanguageModelChartState( prefixScore, lmState, hypo );
  return res;
}

void LanguageModelImplementation::updateChartScore( float *prefixScore, float *finalizedScore, float score, size_t wordPos ) const {
  if (wordPos < GetNGramOrder()) {
    *prefixScore += score;
  }
  else {
    *finalizedScore += score;
  }
}

}
