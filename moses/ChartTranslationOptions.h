/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#pragma once

#include "StackVec.h"
#include "TargetPhrase.h"
#include "TargetPhraseCollection.h"
#include "WordsRange.h"

#include "util/check.hh"
#include <vector>

namespace Moses
{

/** Similar to a DottedRule, but contains a direct reference to a list
 * of translations and provdes an estimate of the best score. For a specific range in the input sentence
 */
class ChartTranslationOptions
{
 public:
  /** Constructor
      \param targetPhraseColl @todo dunno
      \param stackVec @todo dunno
      \param wordsRange the range in the source sentence this translation option covers
      \param score @todo dunno
   */
  ChartTranslationOptions(const TargetPhraseCollection &targetPhraseColl,
                         const StackVec &stackVec,
                         const WordsRange &wordsRange,
                         float score)
      : m_stackVec(stackVec)
      , m_targetPhraseCollection(&targetPhraseColl)
      , m_wordsRange(&wordsRange)
      , m_estimateOfBestScore(score) 
  {}

  ~ChartTranslationOptions() {}

  static float CalcEstimateOfBestScore(const TargetPhraseCollection &,
                                       const StackVec &);

  //! @todo dunno
  const StackVec &GetStackVec() const { return m_stackVec; }

  //! @todo isn't the translation suppose to just contain 1 target phrase, not a whole collection of them?
  const TargetPhraseCollection &GetTargetPhraseCollection() const { 
    return *m_targetPhraseCollection;
  }

  //! the range in the source sentence this translation option covers
  const WordsRange &GetSourceWordsRange() const {
    return *m_wordsRange;
  }

  /** return an estimate of the best score possible with this translation option.
    * the estimate is the sum of the top target phrase's estimated score plus the
    * scores of the best child hypotheses.
    */
  inline float GetEstimateOfBestScore() const { return m_estimateOfBestScore; }

 private:

  StackVec m_stackVec; //! vector of hypothesis list!
  const TargetPhraseCollection *m_targetPhraseCollection;
  const WordsRange *m_wordsRange;
  float m_estimateOfBestScore;
};

}
