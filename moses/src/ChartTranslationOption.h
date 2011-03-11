// $Id$
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

#include <cassert>
#include <vector>
#include "Word.h"
#include "WordsRange.h"
#include "TargetPhrase.h"

namespace Moses
{
class CoveredChartSpan;

// basically a phrase translation and the vector of words consumed to map each word
class ChartTranslationOption
{
  friend std::ostream& operator<<(std::ostream&, const ChartTranslationOption&);

protected:
  const TargetPhrase &m_targetPhrase;
  const CoveredChartSpan &m_lastCoveredChartSpan;
  /* map each source word in the phrase table to:
  		1. a word in the input sentence, if the pt word is a terminal
  		2. a 1+ phrase in the input sentence, if the pt word is a non-terminal
  */
  std::vector<size_t> m_coveredChartSpanListTargetOrder;
  /* size is the size of the target phrase.
  	Usually filled with NOT_KNOWN, unless the pos is a non-term, in which case its filled
  	with its index
  */
  const WordsRange	&m_wordsRange;

  ChartTranslationOption(const ChartTranslationOption &copy); // not implmenented

public:
  ChartTranslationOption(const TargetPhrase &targetPhrase, const CoveredChartSpan &lastCoveredChartSpan, const WordsRange	&wordsRange)
    :m_targetPhrase(targetPhrase)
    ,m_lastCoveredChartSpan(lastCoveredChartSpan)
    ,m_wordsRange(wordsRange)
  {}
  ~ChartTranslationOption()
  {}

  const TargetPhrase &GetTargetPhrase() const {
    return m_targetPhrase;
  }

  const CoveredChartSpan &GetLastCoveredChartSpan() const {
    return m_lastCoveredChartSpan;
  }
  const std::vector<size_t> &GetCoveredChartSpanTargetOrder() const {
    return m_coveredChartSpanListTargetOrder;
  }

  void CreateNonTermIndex();

  const WordsRange &GetSourceWordsRange() const {
    return m_wordsRange;
  }

  inline float GetTotalScore() const {
    return m_targetPhrase.GetFutureScore();
  }

};

}
