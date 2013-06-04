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

#include "moses/ChartCellLabel.h"

namespace Moses
{

/** @todo what is this?
 */
class DottedRule
{
public:
  // used only to init dot stack.
  DottedRule()
    : m_cellLabel(NULL)
    , m_prev(NULL) {}

  DottedRule(const ChartCellLabel &ccl, const DottedRule &prev)
    : m_cellLabel(&ccl)
    , m_prev(&prev) {}

  const WordsRange &GetWordsRange() const {
    return m_cellLabel->GetCoverage();
  }
  const Word &GetSourceWord() const {
    return m_cellLabel->GetLabel();
  }
  bool IsNonTerminal() const {
    return m_cellLabel->GetLabel().IsNonTerminal();
  }
  const DottedRule *GetPrev() const {
    return m_prev;
  }
  bool IsRoot() const {
    return m_prev == NULL;
  }
  const ChartCellLabel &GetChartCellLabel() const {
    return *m_cellLabel;
  }

private:
  const ChartCellLabel *m_cellLabel; // usually contains something, unless
  // it's the init processed rule
  const DottedRule *m_prev;
};

}
