// $Id$
#include <vector>

#include "ScoreComponentCollection.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{

ScoreComponentCollection::ScoreIndexMap ScoreComponentCollection::s_scoreIndexes;
size_t ScoreComponentCollection::s_denseVectorSize = 0;

ScoreComponentCollection::ScoreComponentCollection() : m_scores(s_denseVectorSize)
{}


void ScoreComponentCollection::RegisterScoreProducer
  (const ScoreProducer* scoreProducer) 
{
  assert(scoreProducer->GetNumScoreComponents() != ScoreProducer::unlimited);
  size_t start = s_denseVectorSize;
  size_t end = start + scoreProducer->GetNumScoreComponents();
  VERBOSE(1,"ScoreProducer: " << scoreProducer->GetScoreProducerDescription() << " start: " << start << " end: " << end << endl);
  s_scoreIndexes[scoreProducer] = pair<size_t,size_t>(start,end);
  s_denseVectorSize = end;
}

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

void ScoreComponentCollection::Save(string filename) {
  ofstream out(filename.c_str());
  if (!out) {
    ostringstream msg;
    msg << "Unable to open " << filename;
    throw runtime_error(msg.str());
  }

  ScoreIndexMap::const_iterator iter = s_scoreIndexes.begin();
  for (; iter != s_scoreIndexes.end(); ++iter ) {
	const vector<FName> featureNames = iter->first->GetFeatureNames();
	IndexPair ip = iter->second; // feature indices
	size_t f_index = ip.first;
	for (size_t i=0; (i < featureNames.size()) && (f_index < ip.second); ++i) {
		out << featureNames[i] << " " << m_scores.getCoreFeature(f_index) << endl;
		++f_index;
	}
  }

  // write sparse features
  m_scores.write(out);
  out.close();
}

void ScoreComponentCollection::Assign(const ScoreProducer* sp, const string line) {
  assert(sp->GetNumScoreComponents() == ScoreProducer::unlimited);
  istringstream istr(line);
  while(istr) {
    string namestring;
    FValue value;
    istr >> namestring;
    if (!istr) break;
    istr >> value;
    FName fname(sp->GetScoreProducerDescription(), namestring);
    m_scores[fname] = value;
  }
}

}


