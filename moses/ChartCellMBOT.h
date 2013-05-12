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
class ChartTranslationOptionList;
class ChartCellCollectionMBOT;
class ChartManager;

//Fabienne Braune : nothing to inherit here
class ChartCellBaseMBOT
{

public:
   ChartCellBaseMBOT(size_t startPos, size_t endPos);

   virtual ~ChartCellBaseMBOT();

   const ChartCellLabelSetMBOT &GetTargetLabelSet() const { return m_mbotTargetLabelSet; }

   const ChartCellLabelMBOT &GetSourceWordLabel() const {
        CHECK(m_mbotCoverage.size()!=0);
        CHECK(m_mbotCoverage.front().GetNumWordsCovered() == 1);
        return *m_mbotSourceWordLabel;
      }

   ChartCellLabelSetMBOT &MutableTargetLabelSet() { return m_mbotTargetLabelSet; }

   const std::vector<WordsRange> &GetCoverage() const { return m_mbotCoverage; }

 protected:
   std::vector<WordsRange> m_mbotCoverage;
   ChartCellLabelSetMBOT m_mbotTargetLabelSet;
   ChartCellLabelMBOT *m_mbotSourceWordLabel;
};


class ChartCellMBOT : public ChartCell, public ChartCellBaseMBOT
{
  friend std::ostream& operator<<(std::ostream&, const ChartCellMBOT&);

  //Fabienne Braune : TODO : implement Boost unordered map instead of stl map
  typedef std::map<std::vector<Word>, ChartHypothesisCollection> MapTypeMBOT;

protected:
  MapTypeMBOT m_mbotHypoColl;

  //Fabienne Braune : See if we need an MBOT Chart Manager here...
  ChartManager &m_manager;

public:
  ChartCellMBOT(size_t startPos, size_t endPos, ChartManager &manager);
  ~ChartCellMBOT();

  void ProcessSentence(const ChartTranslationOptionList &transOptList
                       ,const ChartCellCollectionMBOT &allChartCells);

  void ProcessSentenceWithSourceLabels(const ChartTranslationOptionList &transOptList
                          ,const ChartCellCollectionMBOT &allChartCells, const InputType &source, size_t startPos, size_t endPos);

  // Get out target phrases that do not match source label
  void MarkPhrasesWithSourceLabels(std::vector<Word> sourceLabel);

  //Fabienne Braune : rename this into GetAllSortedHypotheses
  const HypoList *GetSortedHypothesesMBOT(const Word &constituentLabel) const;

  bool AddHypothesis(ChartHypothesisMBOT *hypo);

  void SortHypotheses();
  void PruneToSize();

  const ChartHypothesisMBOT *GetBestHypothesis() const;

  void CleanupArcList();

  void OutputSizes(std::ostream &out) const;
  size_t GetSize() const;

  //Fabienne Braune : just in case < is called to compare nonMBOT chart cells
  inline bool operator<(const ChartCell &compare) const {

     std::cout << "Comparison operator for non mbot cells NOT IMPLEMENTED in chart cell mbot" << std::endl;
  }

  //! transitive comparison used for adding objects into set
  inline bool operator<(const ChartCellMBOT &compare) const {
    return m_mbotCoverage < compare.m_mbotCoverage;
  }

  void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned,bool> &reachable) const;

};

}

