// $Id: ChartTranslationOption.h,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $
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

#include "TargetPhrase.h"
#include "TargetPhraseCollection.h"
#include "WordsRange.h"

#include "util/check.hh"
#include <vector>

namespace Moses
{

class DottedRule;
class ChartCellCollection;

// Similar to a DottedRule, but contains a direct reference to a list
// of translations and provdes an estimate of the best score.
class ChartTranslationOption
{
 public:

  ChartTranslationOption(const TargetPhraseCollection &targetPhraseColl,
                         const DottedRule &dottedRule,
                         const WordsRange &wordsRange,
                         const ChartCellCollection &allChartCells)
    : m_dottedRule(dottedRule)
    , m_targetPhraseCollection(targetPhraseColl)
    , m_wordsRange(wordsRange)
    , m_estimateOfBestScore(0)
  {
    //std::cout << "new ChartTranslationOption()" << std::endl;
    CalcEstimateOfBestScore(allChartCells);
  }

  ~ChartTranslationOption() {}

  const DottedRule &GetDottedRule() const { return m_dottedRule; }

  const TargetPhraseCollection &GetTargetPhraseCollection() const {
    return m_targetPhraseCollection;
  }

  const WordsRange &GetSourceWordsRange() const {
    return m_wordsRange;
  }

  // return an estimate of the best score possible with this translation option.
  // the estimate is the sum of the top target phrase's estimated score plus the
  // scores of the best child hypotheses.
    inline float GetEstimateOfBestScore() const { return m_estimateOfBestScore; }



 private:
  // not implemented
  ChartTranslationOption &operator=(const ChartTranslationOption &);

  void CalcEstimateOfBestScore(const ChartCellCollection &);

  const DottedRule &m_dottedRule;
  const TargetPhraseCollection &m_targetPhraseCollection;
  const WordsRange &m_wordsRange;
  float m_estimateOfBestScore;
};

}
