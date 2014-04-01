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

#include <vector>
#include "RuleCubeItem.h"
//#include "TargetPhraseMBOT.h"

namespace Moses
{
class ChartCellCollectionMBOT;
class ChartHypothesisMBOT;
class ChartManager;
class DottedRuleMBOT;
class ChartTranslationOptionMBOT;
class TargetPhraseMBOT;

// wrapper around list of hypotheses for a particular non-term of a trans opt
class HypothesisDimensionMBOT
{
public:
  HypothesisDimensionMBOT(std::size_t pos, const HypoListMBOT &orderedHypos)
    : m_pos(pos)
    , m_orderedHypos(&orderedHypos)
  {}

 ~HypothesisDimensionMBOT()
 {};

  std::size_t IncrementPos() {
        return m_pos++;
}

  bool HasMoreHypo() const {
    return m_pos+1 < m_orderedHypos->size();
  }

  const ChartHypothesisMBOT *GetHypothesis() const {
    return (*m_orderedHypos)[m_pos];
  }

  bool operator<(const HypothesisDimensionMBOT &compare) const {
    return GetHypothesis() < compare.GetHypothesis();
  }

  bool operator==(const HypothesisDimensionMBOT &compare) const {
    return GetHypothesis() == compare.GetHypothesis();
  }

private:
  std::size_t m_pos;
  const HypoListMBOT *m_orderedHypos;
};


class RuleCubeItemMBOT : public RuleCubeItem
{
 public:
  RuleCubeItemMBOT(const ChartTranslationOptionMBOT &, const ChartCellCollection &);
  RuleCubeItemMBOT(const RuleCubeItemMBOT &, int);
  ~RuleCubeItemMBOT();

    const TranslationDimension &GetTranslationDimension() const {
        std::cout << "Get non mbot translation dimension NOT IMPLEMENTED in translation dimension MBOT" << std::endl;
  }

  const TranslationDimension &GetTranslationDimensionMBOT() const {
        return m_mbotTranslationDimension;
  }

  void IncrementTranslationDimension() {
         m_mbotTranslationDimension.IncrementPos();
   }

  const std::vector<HypothesisDimension> &GetHypothesisDimensions() const {
    std::cout << "Get non mbot hypothesis dimension NOT IMPLEMENTED in translation dimension MBOT" << std::endl;
  }

  const std::vector<HypothesisDimensionMBOT> &GetHypothesisDimensionsMBOT() const {
    return m_mbotHypothesisDimensions;
  }

  float GetScore() const {
   std::cout << "Get non mbot score NOT IMPLEMENTED in translation dimension MBOT" << std::endl;
  }

  float GetScoreMBOT() const { return m_mbotScore; }

  void EstimateScore()
  {
      std::cout << "Estimate Score for non mbot cube item NOT IMPLEMENTED in rule cube item mbot" << std::endl;
  }

  void EstimateScoreMBOT();

  void CreateHypothesis(const ChartTranslationOptionMBOT &, ChartManager &);

  ChartHypothesisMBOT *ReleaseHypothesisMBOT();


 bool operator<(const RuleCubeItemMBOT &) const;

 private:
  RuleCubeItemMBOT(const RuleCubeItemMBOT &);  // Not implemented
  RuleCubeItemMBOT &operator=(const RuleCubeItemMBOT &);  // Not implemented
  void CreateHypothesisDimensionsMBOT(const DottedRuleMBOT &, const ChartCellCollection&);

 protected :
   TranslationDimension m_mbotTranslationDimension;
  std::vector<HypothesisDimensionMBOT> m_mbotHypothesisDimensions;
  ChartHypothesisMBOT *m_mbotHypothesis;
  float m_mbotScore;
};
}
