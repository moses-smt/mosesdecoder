// $Id$
#include <vector>

#include "ScoreComponentCollection.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{
ScoreComponentCollection::ScoreComponentCollection()
{}


float ScoreComponentCollection::GetWeightedScore() const
{
	return m_scores.inner_product(StaticData::Instance().GetAllWeights().m_scores);
}
		
void ScoreComponentCollection::ZeroAllLM(const LMList& lmList)
{
  for (LMList::const_iterator i = lmList.begin(); i != lmList.end(); ++i) {
    Assign(*i, 0);
  }
}

void ScoreComponentCollection::PlusEqualsAllLM(const LMList& lmList, const ScoreComponentCollection& rhs)
{
  for (LMList::const_iterator i = lmList.begin(); i != lmList.end(); ++i) {
    PlusEquals(*i,rhs);
  }
}

	
void ScoreComponentCollection::MultiplyEquals(float scalar)
{
	m_scores *= scalar;
}

void ScoreComponentCollection::DivideEquals(float scalar)
{
	m_scores /= scalar;
}
	
	
void ScoreComponentCollection::MultiplyEquals(const ScoreComponentCollection& rhs)
{
	m_scores *= rhs.m_scores;
}

std::ostream& operator<<(std::ostream& os, const ScoreComponentCollection& rhs)
{
	os << "<<" << rhs.m_scores;
	return os << ">>";
}
void ScoreComponentCollection::L1Normalise() {
  m_scores /= m_scores.l1norm();
}

float ScoreComponentCollection::GetL1Norm() {
  return m_scores.l1norm();
}

float ScoreComponentCollection::GetL2Norm() {
  return m_scores.l2norm();
}

}


