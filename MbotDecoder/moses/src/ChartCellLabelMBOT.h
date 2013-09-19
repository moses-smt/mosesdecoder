/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "Word.h"
#include "WordsRange.h"

#include "ChartCellLabel.h"
#include "ChartHypothesisCollectionMBOT.h"

namespace Moses
{

class ChartHypothesisCollection;
class Word;

class ChartCellLabelMBOT : public ChartCellLabel
{
 friend std::ostream& operator<<(std::ostream &, const ChartCellLabelMBOT &);

  public:
  ChartCellLabelMBOT(const WordsRange &coverage, const Word &label,
                 const ChartHypothesisCollectionMBOT *stack=NULL)
                 : ChartCellLabel(coverage,label, stack)
                 , m_mbotStack(stack)
  {

      m_mbotCoverage.push_back(coverage);
      m_mbotLabel.push_back(label);
  }

  ~ChartCellLabelMBOT(){

    m_mbotCoverage.clear();
    m_mbotCoverage.clear();
    m_mbotLabel.clear();
  }

  void AddCoverage(WordsRange coverage)
  {
      m_mbotCoverage.push_back(coverage);
  }

  void AddLabel(Word label)
  {
      m_mbotLabel.push_back(label);
  }

  const std::vector<WordsRange> &GetCoverageMBOT() const { return m_mbotCoverage; }


  const std::vector<Word> &GetLabelMBOT() const {
      return m_mbotLabel;
      }


  const ChartHypothesisCollectionMBOT *GetStackMBOT() const { return m_mbotStack; }

  bool CompareLabels(std::vector<Word> other) const
  {
        if(m_mbotLabel.size() != other.size())
        {
        return m_mbotLabel.size() < other.size();
        }
       //first element is considered as smallest
        std::vector<Word> :: const_iterator itr_label;
        std::vector<Word> :: const_iterator itr_labelOther;
        for(itr_label = m_mbotLabel.begin(), itr_labelOther = other.begin();
            itr_label != m_mbotLabel.end(), itr_labelOther != other.end(); itr_label++, itr_labelOther++)
        {
             Word current = *itr_label;
             Word other = *itr_labelOther;

            if(current != other)
            {
                return current < other;
            }
         }
         //if all words in vector are equal then return comparison of front elements
         return false;
}

  bool operator<(const ChartCellLabel &other) const
  {
     std::cout << "Comparison between non mbot Chart Cell Labels NOT IMPLEMENTED in Chart Cell Label mbot" << std::endl;
  }

  bool operator<(const ChartCellLabelMBOT &other) const
  {
    //compare two smallest coverage vectors = compare smallest element of each vector
    if(m_mbotCoverage.size() != other.m_mbotCoverage.size())
    {
        return m_mbotCoverage.size() < other.m_mbotCoverage.size();
    }
    std::vector<WordsRange>::const_iterator itr_coverage;
    std::vector<WordsRange>::const_iterator itr_coverageOther;
        for(
                itr_coverage = m_mbotCoverage.begin(), itr_coverageOther = other.m_mbotCoverage.begin();
                itr_coverage != m_mbotCoverage.end(), itr_coverageOther != other.m_mbotCoverage.end();
                itr_coverage++, itr_coverageOther++) //itr_label++, itr_labelOther++)
                {
                    WordsRange currentRange1 = *itr_coverage;
                    WordsRange currentRange2 = *itr_coverageOther;

                    if(currentRange1 == currentRange2)
                    {
                        return CompareLabels(other.GetLabelMBOT());
                    }
                    else
                    {
                        return currentRange1 < currentRange2;
                    }
                }
    }

 private:
    std::vector<WordsRange> m_mbotCoverage;
    std::vector<Word> m_mbotLabel;
    const ChartHypothesisCollectionMBOT *m_mbotStack;
};

}
