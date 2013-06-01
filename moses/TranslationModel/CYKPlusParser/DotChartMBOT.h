//Fabienne Braune
//Dotted rules for l-MBOT decoder
//Needed to extend DotChart.h for dealing with
//specific cell labels having a sequence of labels instead of just one.
//For more comments, see class ChartCellLabelMBOT


#pragma once

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "moses/ChartCellLabelMBOT.h"
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
      {}

  DottedRuleMBOT(const ChartCellLabelMBOT &ccl, const DottedRuleMBOT &prev)
      : m_mbotCellLabel(&ccl)
      , m_mbotPrev(&prev)
       {}

  virtual void SetSourceLabel(const Word sl)
  {
      m_mbotSourceCellLabel = sl;
  }

  virtual const Word GetSourceLabel() const
  {
      return m_mbotSourceCellLabel;
  }

  //Fabienne Braune : created own methods instead of using inheritance. When extending to rules applying to several source
  //spans (l-MBOT backwards application) we need these methods to return a vector of words range. For now (l_MBOT forward
  //application), we just take the first element of the vector.

  //Fabienne Braune : just making sure that this is not called by mistake
  const WordsRange &GetWordsRange() const {
    std::cout << "Get words range of non mbot dotted rule NOT IMPLEMENTED in dotted rule mbot" << std::endl;
    }

  const WordsRange &GetWordsRangeMBOT() const { return m_mbotCellLabel->GetCoverageMBOT().front(); }

  const Word &GetSourceWord() const {
   std::cout << "Get source word of non mbot dotted rule NOT IMPLEMENTED in dotted rule mbot" << std::endl;
  }

  //FB : for source words there is only one element in the cellLabel, so take first element of vector
  const Word GetSourceWordMBOT() const {
  return *(m_mbotCellLabel->GetLabelMBOT().GetWord(0)); }

  bool IsNonTerminal() const {
   std::cout << "Check if non terminal of non mbot dotted rule NOT IMPLEMENTED in dotted rule mbot" << std::endl;
  }

  bool IsNonTerminalMBOT() const { return m_mbotCellLabel->GetLabelMBOT().GetWord(0)->IsNonTerminal(); }

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

  const ChartCellLabelMBOT * GetChartCellLabelMBOT() const { return m_mbotCellLabel; }

 private:
  const ChartCellLabelMBOT *m_mbotCellLabel; // usually contains something, unless
                                     // it's the init processed rule
  const DottedRuleMBOT *m_mbotPrev;

  //Fabienne Braune : used for debugging. Can be removed.
  Word m_mbotSourceCellLabel;

};


}
