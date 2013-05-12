//Fabienne Braune : A chart Cell with several labels for modeling sequences of non-terminals on target side

//Comments on implementation :
//A chartCellMBOT is a collection of chart Cells in the sense that an l-MBOT rule plugs its discontiguous non-terminals into
//several chart cells. So an l-MBOT chart Cell has several coverages and labels (m_mbotCoverage and m_mbotLabel)
//However, as all non-terminals are plugged-in at the same time, each l-MBOT rule has a single stack of chart hypotheses

#pragma once

#include "HypoList.h"
#include "Word.h"
#include "WordsRange.h"
#include "ChartCellLabel.h"

namespace search { class Vertex; }

namespace Moses
{

class ChartHypothesisCollection;
class Word;

class ChartCellLabelMBOT : public ChartCellLabel
{

public:
  /*union Stack {
    const HypoList *cube; // cube pruning
    search::Vertex *incr; // incremental search after filling.
    void *incr_generator; // incremental search during filling.
  };*/


 friend std::ostream& operator<<(std::ostream &, const ChartCellLabelMBOT &);

  public:
  ChartCellLabelMBOT(const WordsRange &coverage, const Word &label,
                 const Stack stack=Stack())
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

  const WordsRange &GetCoverage() const
  {
    std::cout << "Get coverage of non mbot Chart Cell Label NOT IMPLEMENTED in Chart Cell Label MBOT" << std::endl;
  }

  const std::vector<WordsRange> &GetCoverageMBOT() const { return m_mbotCoverage; }


  const Word &GetLabel() const
  {
    std::cout << "Get label of non mbot Chart Cell Label NOT IMPLEMENTED in Chart Cell Label MBOT" << std::endl;
  }

  const std::vector<Word> &GetLabelMBOT() const {
      return m_mbotLabel;
      }

  const Stack GetStackMBOT() const { return m_mbotStack; }

  Stack&MutableMBOTStack() { return m_mbotStack; };


  const Stack *GetStack() const {
      std::cout << "Get Chart Hypothesis Collection NOT implemented in chart cell mbot" << std::endl;
      }

  bool CompareLabels(std::vector<Word> other) const
  {
        if(m_mbotLabel.size() != other.size())
        {
        	return m_mbotLabel.size() < other.size();
        }

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

         return false;
}

  //Fabienne Braune : Just in case this gets called instead of < beteween MBOT chart cell labels
  bool operator<(const ChartCellLabel &other) const
  {
     std::cout << "Comparison between non mbot Chart Cell Labels NOT IMPLEMENTED in Chart Cell Label mbot" << std::endl;
  }

  bool operator<(const ChartCellLabelMBOT &other) const
  {

    if(m_mbotCoverage.size() != other.m_mbotCoverage.size())
    {
        return m_mbotCoverage.size() < other.m_mbotCoverage.size();
    }

    std::vector<WordsRange>::const_iterator itr_coverage;
    std::vector<WordsRange>::const_iterator itr_coverageOther;

        for(
                itr_coverage = m_mbotCoverage.begin(), itr_coverageOther = other.m_mbotCoverage.begin();
                itr_coverage != m_mbotCoverage.end(), itr_coverageOther != other.m_mbotCoverage.end();
                itr_coverage++, itr_coverageOther++)
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
    Stack m_mbotStack;

};

}
