//Fabienne Braune
//ChartCell for containing l-MBOT rules

#pragma once

#include <iostream>
#include <queue>
#include <map>
#include <vector>
#include "Word.h"
#include "WordsRange.h"
#include "NonTerminal.h"
#include "ChartHypothesisMBOT.h"
#include "RuleCube.h"
#include "ChartCellLabelSetMBOT.h"
#include "ChartCell.h"

namespace Moses
{

class ChartCellMBOT : public ChartCell {
  friend std::ostream& operator<<(std::ostream&, const ChartCellMBOT&);

  //Fabienne Braune : TODO : implement Boost unordered map instead of stl map
  typedef std::map<WordSequence, ChartHypothesisCollection> MapTypeMBOT;

protected:
  MapTypeMBOT m_mbotHypoColl;

public:
  ChartCellMBOT(size_t startPos, size_t endPos, ChartManager &manager);
  ~ChartCellMBOT();

  void ProcessSentenceWithMBOT(const ChartTranslationOptionList &transOptList
                       ,const ChartCellCollection &allChartCells);

  void ProcessSentenceWithSourceLabels(const ChartTranslationOptionList &transOptList
                          ,const ChartCellCollection &allChartCells, const InputType &source, size_t startPos, size_t endPos);

  // Get out target phrases that do not match source label
  void MarkPhrasesWithSourceLabels(std::vector<Word> sourceLabel);

  const HypoList *GetSortedHypotheses(const Word &constituentLabel) const
  {
	  std::cerr << "ERROR : NOT YET IMPLEMENTED" << std::endl;
  }

  //! for n-best list
  const HypoList *GetAllSortedHypotheses() const
  {
	  std::cerr << "TO IMPLEMENT..."<< std::endl;
	  abort();
  }

  bool AddHypothesis(ChartHypothesisMBOT *hypo);

  void SortHypotheses();
  void PruneToSize();

  const ChartHypothesisMBOT *GetBestHypothesisMBOT() const;

  void CleanupArcList();

  void OutputSizes(std::ostream &out) const;
  size_t GetSize() const;

  void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned,bool> &reachable) const;

};

}

