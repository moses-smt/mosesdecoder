// $Id: DotChartMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "ChartCellLabelMBOT.h"
#include "DotChart.h"

namespace Moses
{

class DottedRuleMBOT : public DottedRule
{
  friend std::ostream& operator<<(std::ostream &, const DottedRuleMBOT &);

 public:
  // used only to init dot stack.
  DottedRuleMBOT()
      : m_mbotCellLabel(NULL)
      , m_mbotPrev(NULL)
      {//std::cout<<"new DottedRuleMBOT() " << this << std::endl;
      }

  DottedRuleMBOT(const ChartCellLabelMBOT &ccl, const DottedRuleMBOT &prev)
      : m_mbotCellLabel(&ccl)
      , m_mbotPrev(&prev)
       {
          //std::cout<<"new DottedRuleMBot(2) " << this << std::endl;
          }

  //accessors to source label

  virtual void SetSourceLabel(const Word sl)
  {
      m_mbotSourceCellLabel = sl;
  }

  virtual const Word GetSourceLabel() const
  {
      return m_mbotSourceCellLabel;
  }

  const WordsRange &GetWordsRange() const {
    std::cout << "Get words range of non mbot dotted rule NOT IMPLEMENTED in dotted rule mbot" << std::endl;
    }

  const WordsRange &GetWordsRangeMBOT() const { return m_mbotCellLabel->GetCoverageMBOT().front(); }

  const Word &GetSourceWord() const {
   std::cout << "Get source word of non mbot dotted rule NOT IMPLEMENTED in dotted rule mbot" << std::endl;
  }

  //FB : for source words there is only one element in the cellLabel, so take first element of vector
  const Word &GetSourceWordMBOT() const {
  return m_mbotCellLabel->GetLabelMBOT().front(); }

  bool IsNonTerminal() const {
   std::cout << "Check if non terminal of non mbot dotted rule NOT IMPLEMENTED in dotted rule mbot" << std::endl;
  }

  bool IsNonTerminalMBOT() const { return m_mbotCellLabel->GetLabelMBOT().front().IsNonTerminal(); }


  const DottedRule *GetPrev() const {
    std::cout << "Get previous of non mbot dotted rule NOT IMPLEMENTED in dotted rule mbot" << std::endl;
    return GetPrevMBOT();
  }

  const DottedRuleMBOT *GetPrevMBOT() const { return m_mbotPrev; }

  virtual bool IsRoot() const {std::cout << "Check if root NOT IMPLEMENTED in dotted rule mbot" << std::endl; return m_mbotPrev == NULL; }


  bool IsRootMBOT() const { return m_mbotPrev == NULL; }

  const ChartCellLabel &GetChartCellLabel() const {
    std::cout << "Get chart cell label NOT IMPLEMENTED in dotted rule mbot" << std::endl;
  }

  const ChartCellLabelMBOT &GetChartCellLabelMBOT() const { return *m_mbotCellLabel; }

 private:
  const ChartCellLabelMBOT *m_mbotCellLabel; // usually contains something, unless
                                     // it's the init processed rule
  const DottedRuleMBOT *m_mbotPrev;

  //FB : used for debugging : see which Word has been used as source label
  Word m_mbotSourceCellLabel;

};


}
