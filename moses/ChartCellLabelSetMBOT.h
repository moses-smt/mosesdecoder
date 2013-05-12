//Fabienne Braune
//Set of MBOT Cell Labels. More details about CellLabel in ChartCellLabelMBOT

#pragma once

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "ChartCellLabelMBOT.h"
#include "ChartCellLabelSet.h"

#include <set>

namespace Moses
{

class ChartHypothesisCollection;

class ChartCellLabelSetMBOT
{

 //Fabienne Braune : I use an stl map for now
 //todo : switch to boost unordered_map but then define a non-terminal map as a map between a VECTOR of words and a chart cell label
 private:
  typedef std::map<std::vector<Word>, ChartCellLabelMBOT> MapTypeMBOT;

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

  //Fabienne Braune : Make label with first word and then add the other ones
  void AddWord(const std::vector<Word> &w)
  {
    ChartCellLabelMBOT cellLabel(m_mbotCoverage.front(), w.front());
    std::vector<Word>::const_iterator itr_word;
    int counter = 1;
    for(itr_word = w.begin()+1; itr_word!=w.end();itr_word++)
    {
        cellLabel.AddLabel(*itr_word);
        if(counter < m_mbotCoverage.size())
        {cellLabel.AddCoverage(m_mbotCoverage[counter++]);}
    }
    m_mbotMap.insert(std::make_pair(w,cellLabel));
  }

  void AddConstituent(const std::vector<Word> &w, const HypoList *stack)
  {
    std::vector<Word>::const_iterator itr_word;

    ChartCellLabelMBOT::Stack s;
    s.cube = stack;

    ChartCellLabelMBOT cellLabel = ChartCellLabelMBOT(m_mbotCoverage.front(), w.front(), s);
    int counter = 1;
    for(itr_word = w.begin()+1; itr_word!=w.end();itr_word++)
    {
        cellLabel.AddLabel(*itr_word);
        if(counter < m_mbotCoverage.size())
        {
            cellLabel.AddCoverage(m_mbotCoverage[counter++]);
        }
    }
    m_mbotMap.insert(std::make_pair(w,cellLabel));
  }


  bool Empty() const { return m_mbotMap.empty(); }

  size_t GetSize() const { return m_mbotMap.size(); }

  const ChartCellLabelMBOT *Find(const std::vector<Word> &w) const
  {
	 //Fabienne Braune : Construct with first elements and add the other ones later
	 ChartCellLabelMBOT CellToFind = ChartCellLabelMBOT(m_mbotCoverage.front(), w.front());
     std::vector<Word>::const_iterator itr_word;

     int counter = 1;
     for(itr_word = w.begin()+1; itr_word!=w.end();itr_word++)
     {
        CellToFind.AddLabel(*itr_word);
        if(counter < m_mbotCoverage.size())
        {CellToFind.AddCoverage(m_mbotCoverage[counter++]);}
    }

    MapTypeMBOT::const_iterator p = m_mbotMap.find(w);
    return p == m_mbotMap.end() ? 0 : &(p->second);
  }

  ChartCellLabelMBOT::Stack &FindOrInsert(const std::vector<Word> &w) {
    return m_mbotMap.insert(std::make_pair(w,ChartCellLabelMBOT(m_mbotCoverage.front(), w.front()))).first->second.MutableStack();
  }

 private:
  std::vector<WordsRange> m_mbotCoverage;
  MapTypeMBOT m_mbotMap;
};

}
