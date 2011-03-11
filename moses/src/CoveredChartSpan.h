// $Id$
// vim:tabstop=2
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

#include <iostream>
#include "WordsRange.h"
#include "Word.h"

namespace Moses
{

class CoveredChartSpan
{
  friend std::ostream& operator<<(std::ostream&, const CoveredChartSpan&);

protected:
  WordsRange	m_coverage;
  const Word &m_sourceWord; // can be non-term or term
  const CoveredChartSpan *m_prevCoveredChartSpan;
public:
  CoveredChartSpan(); // not implmented
  CoveredChartSpan(size_t startPos, size_t endPos, const Word &sourceWord, const CoveredChartSpan *prevCoveredChartSpan)
    :m_coverage(startPos, endPos)
    ,m_sourceWord(sourceWord)
    ,m_prevCoveredChartSpan(prevCoveredChartSpan)
  {}
  const WordsRange &GetWordsRange() const {
    return m_coverage;
  }
  const Word &GetSourceWord() const {
    return m_sourceWord;
  }
  WordsRange &GetWordsRange() {
    return m_coverage;
  }
  bool IsNonTerminal() const {
    return m_sourceWord.IsNonTerminal();
  }

  const CoveredChartSpan *GetPrevCoveredChartSpan() const {
    return m_prevCoveredChartSpan;
  }

  //! transitive comparison used for adding objects into FactorCollection
  inline bool operator<(const CoveredChartSpan &compare) const {
    if (IsNonTerminal() < compare.IsNonTerminal())
      return true;
    else if (IsNonTerminal() == compare.IsNonTerminal())
      return m_coverage < compare.m_coverage;

    return false;
  }

};

}; // namespace
