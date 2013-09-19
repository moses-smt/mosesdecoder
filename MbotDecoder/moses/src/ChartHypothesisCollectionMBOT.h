// $Id: ChartHypothesisCollectionMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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

#include <set>
#include "ChartHypothesisCollection.h"
#include "ChartHypothesisMBOT.h"
#include "RuleCubeMBOT.h"


namespace Moses
{

class ChartHypothesisScoreOrdererMBOT : public ChartHypothesisScoreOrderer
{
public:
  virtual bool operator()(const ChartHypothesisMBOT* hypoA, const ChartHypothesisMBOT* hypoB) const {
      //std::cout << "Comparing scores : " << hypoA->GetTotalScore() << " : " << hypoB->GetTotalScore() << std::endl;
    return hypoA->GetTotalScore() > hypoB->GetTotalScore();
  }
};

class ChartHypothesisRecombinationOrdererMBOT : public ChartHypothesisRecombinationOrderer
{
public:

  //std::cout << "Recombination orderer : " << std::endl;
  bool operator()(const ChartHypothesisMBOT* hypoA, const ChartHypothesisMBOT* hypoB) const {
    // assert in same cell
    // std::cout << "Recombination orderer : " << std::endl;

         //PRINT COMPARED HYPOS
    //std::cout << "PRINT COMPARED HYPOS : " << std::endl;
    //std::cout << "Comparing Hypos (1) : " << (*hypoA) << std::endl;
    //std::cout << "Comparing Hypos (2) : " << (*hypoB) << std::endl;

    const WordsRange &rangeA	= hypoA->GetCurrSourceRange()
                                      , &rangeB	= hypoB->GetCurrSourceRange();
    //std::cout << "Compare : " << rangeA << " : " << rangeB << std::endl;
    CHECK(rangeA == rangeB);

    //std::cout << "Comparre LHS" << std::endl;
    CHECK(hypoA->GetTargetLHSMBOT() == hypoB->GetTargetLHSMBOT());

    int ret = hypoA->RecombineCompare(*hypoB);
    if (ret != 0)
      return (ret < 0);

    return false;
  }
};

// 1 of these for each target LHS in each cell
class ChartHypothesisCollectionMBOT : public ChartHypothesisCollection
{
  friend std::ostream& operator<<(std::ostream&, const ChartHypothesisCollectionMBOT&);

protected:
  typedef std::set<ChartHypothesisMBOT*, ChartHypothesisRecombinationOrdererMBOT> HCTypeMBOT;
  HCTypeMBOT m_mbotHypos;
  HypoListMBOT m_mbotHyposOrdered;

  /** add hypothesis to stack. Prune if necessary.
   * Returns false if equiv hypo exists in collection, otherwise returns true
   */
  std::pair<HCTypeMBOT::iterator, bool> Add(ChartHypothesisMBOT *hypo, ChartManager &manager);

public:
  typedef HCTypeMBOT::iterator iterator;
  typedef HCTypeMBOT::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const {
    return m_mbotHypos.begin();
  }
  const_iterator end() const {
    return m_mbotHypos.end();
  }

  ChartHypothesisCollectionMBOT();
  ~ChartHypothesisCollectionMBOT();

  bool AddHypothesis(ChartHypothesisMBOT *hypo, ChartManager &manager);

  //!n forbid adding non-mbot chart hypothesis
  bool AddHypothesis(ChartHypothesis *hypo, ChartManager &manager)
  {
      std::cout << "Non mbot hypothesis NOT IMPLEMENTED in ChartHypothesisCollectionMBOT" << std::endl;
  }

  //! remove hypothesis pointed to by iterator but don't delete the object
  void Detach(const HCTypeMBOT::iterator &iter);

  //!n forbid detaching non-mbot chart hypothesis
  void Detach(const HCType::iterator &iter)
  {
      std::cout << "Detach non mbot hypothesis NOT implemented in ChartHypothesisCollectionMBOT" << std::endl;
  }

  /** destroy Hypothesis pointed to by iterator (object pool version) */
  void Remove(const HCTypeMBOT::iterator &iter);

   //!n forbid detaching non-mbot chart hypothesis
  void Remove(const HCType::iterator &iter)
  {
      std::cout << "Remove non mbot hypothesis NOT implemented in ChartHypothesisCollectionMBOT" << std::endl;
  }

  void PruneToSize(ChartManager &manager);

  size_t GetSizeMBOT() const {
    return m_mbotHypos.size();
  }

  //! forbid getting size of non-mbot hypotheses
  size_t GetSize() const {
    std::cout << "Get size of non mbot hypothesis NOT implemented in ChartHypothesisCollectionMBOT" << std::endl;
  }

  const HCTypeMBOT GetHypoMBOT() const {
    return m_mbotHypos;
  }

  //! forbid getting non-mbot hypo
  size_t GetHypo() const {
    std::cout << "Get non mbot hypothesis NOT implemented in ChartHypothesisCollectionMBOT" << std::endl;
  }

  void SortHypotheses();
  void CleanupArcList();

  const HypoListMBOT &GetSortedHypothesesMBOT() const {
     //std::cout << "Getting Sorted Hypotheses " << m_mbotHyposOrdered.size()<< std::endl;
    return m_mbotHyposOrdered;
  }

  //! forbid getting non-mbot sorted hypos
  const HypoList &GetSortedHypotheses() const {
     std::cout << "Get non mbot sorted hypotheses NOT implemented in ChartHypothesisCollectionMBOT" << std::endl;
  }

  void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned,bool> &reachable) const;

};

} // namespace

