// $Id: ScoreComponentCollection.cpp,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $

#include "ScoreComponentCollection.h"
#include "StaticData.h"

namespace Moses
{
ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(StaticData::Instance().GetTotalScoreComponents(), 0.0f)
  , m_sim(&StaticData::Instance().GetScoreIndexManager())
{}

float ScoreComponentCollection::GetWeightedScore() const
{
  float ret = InnerProduct(StaticData::Instance().GetAllWeights());
  return ret;
}

void ScoreComponentCollection::ZeroAllLM(const LMList& lmList)
{

  for (size_t ind = lmList.GetMinIndex(); ind <= lmList.GetMaxIndex(); ++ind) {
    m_scores[ind] = 0;
  }
}

void ScoreComponentCollection::PlusEqualsAllLM(const LMList& lmList, const ScoreComponentCollection& rhs)
{

  for (size_t ind = lmList.GetMinIndex(); ind <= lmList.GetMaxIndex(); ++ind) {
    m_scores[ind] += rhs.m_scores[ind];
  }

}

}


