//Fabienne Braune
//Set of MBOT Cell Labels. More details about CellLabel in ChartCellLabelMBOT

#pragma once

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "ChartCellLabelMBOT.h"
#include "ChartCellLabelSet.h"
#include "WordSequence.h"

#include <set>

namespace Moses
{

class ChartHypothesisCollection;

class ChartCellLabelSetMBOT
{

 //Fabienne Braune : I use an stl map for now
 //todo : switch to boost unordered_map but then define a non-terminal map as a map between a VECTOR of words and a chart cell label
 private:
  typedef std::map<WordSequence, ChartCellLabelMBOT> MapTypeMBOT;

 public:
  typedef MapTypeMBOT::const_iterator const_iterator;
  ChartCellLabelSetMBOT(const WordsRange coverage)
  {
    m_mbotCoverage.push_back(coverage);
  }
  const_iterator begin() const { return m_mbotMap.begin(); }
  const_iterator end() const { return m_mbotMap.end(); }

  ~ChartCellLabelSetMBOT(){
  }

  void AddCoverage(WordsRange &coverage)
  {
      m_mbotCoverage.push_back(coverage);
  }

  void AddWord(const WordSequence w)
  {
    int counter = 1;
    m_mbotMap.insert(std::make_pair(w,ChartCellLabelMBOT(m_mbotCoverage, w)));
  }

  void AddConstituent(const WordSequence w, const HypoList *stack)
  {

    ChartCellLabelMBOT::Stack s;
    s.cube = stack;
    m_mbotMap.insert(std::make_pair(w,ChartCellLabelMBOT(m_mbotCoverage, w, s)));
  }


  bool Empty() const { return m_mbotMap.empty(); }

  size_t GetSize() const { return m_mbotMap.size(); }

  const ChartCellLabelMBOT *Find(const WordSequence &w) const
  {
	 ChartCellLabelMBOT CellToFind = ChartCellLabelMBOT(m_mbotCoverage, w);
     WordSequence::const_iterator itr_word;

    MapTypeMBOT::const_iterator p = m_mbotMap.find(w);
    return p == m_mbotMap.end() ? 0 : &(p->second);
  }

  ChartCellLabelMBOT::Stack &FindOrInsert(const WordSequence &w) {
    return m_mbotMap.insert(std::make_pair(w,ChartCellLabelMBOT(m_mbotCoverage, w))).first->second.MutableStack();
  }

 private:
  std::vector<WordsRange> m_mbotCoverage;
  MapTypeMBOT m_mbotMap;
};

}
