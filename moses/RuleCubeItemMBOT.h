//Fabienne Braune : Rule Cube Item dealing with l-MBOT hypotheses

//Comment on implementation : could use RuleCubeItem and implement a method creating an l-MBOT hpyothesis (ChartHypothesisMBOT)
//Found it more modular to write a derived class but this can be easily modified

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
class ChartTranslationOptionMBOT;
class TargetPhraseMBOT;

//Fabienne Braune TODO : Remove this if things work out by reusing normal hypothesis dimension
// wrapper around list of hypotheses for a particular non-term of a trans opt
/*class HypothesisDimensionMBOT
{
public:
  HypothesisDimensionMBOT(std::size_t pos, const HypoListMBOT &orderedHypos)
    : m_pos(pos)
    , m_orderedHypos(&orderedHypos)
  {
    //std::cout << "new HypothesisDimensionMBOT() : " << m_pos << " : "<< m_orderedHypos->size() << std::endl;
  }

 ~HypothesisDimensionMBOT()
 {
     //std::cout << "DESTROYED DIMENSION MBOT"<< std::endl;
 };

  std::size_t IncrementPos() {
  //std::cout << "HYDI : Icrementing position " << m_pos << std::endl;
        return m_pos++;
}

  bool HasMoreHypo() const {
      //std::cout << "Checking for more hypos..." << std::endl;
    return m_pos+1 < m_orderedHypos->size();
  }

  const ChartHypothesisMBOT *GetHypothesis() const {
    //std::cout << "Getting Hypothesis at position : " << m_pos << std::endl;
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
};*/

//new : rule cube item for MBOTS
class RuleCubeItemMBOT : public RuleCubeItem
{
 public:
  RuleCubeItemMBOT(const ChartTranslationOptions &, const ChartCellCollection &);
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

  //Fabienne Braune : Changed HypothesisDimensionMBOT into HypothesisDimension
  const std::vector<HypothesisDimension> &GetHypothesisDimensionsMBOT() const {
    return m_hypothesisDimensions;
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

  void CreateHypothesis(const ChartTranslationOptions &, ChartManager &);

  ChartHypothesisMBOT *ReleaseHypothesisMBOT();


 bool operator<(const RuleCubeItemMBOT &) const;

 //Fabiennne Braune : Renamed private methods with ...MBOT. Would maybe be good to make these methods public in the base class?
 private:
  RuleCubeItemMBOT(const RuleCubeItemMBOT &);  // Not implemented
  RuleCubeItemMBOT &operator=(const RuleCubeItemMBOT &);  // Not implemented
  void CreateHypothesisDimensionsMBOT(const StackVec &stackVec);

 protected :
  TranslationDimension m_mbotTranslationDimension;
  std::vector<HypothesisDimension> m_hypothesisDimensions;
  ChartHypothesisMBOT *m_mbotHypothesis;
  float m_mbotScore;
};
}
