#include "PhraseDistanceFeature.h"

#include <vector>
#include <boost/foreach.hpp>
#include "moses/InputType.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/StaticData.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
PhraseDistanceFeature::PhraseDistanceFeature(const string &line)
  : StatelessFeatureFunction(2, line)
  , m_space("")
  , m_spaceID(0)
  , m_measure(EuclideanDistance)
{
  ReadParameters();
}

void PhraseDistanceFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedScores) const
{
  vector<float> scores(m_numScoreComponents, 0);
  bool broken = false;
  // Input coord
  map<size_t const, vector<float> >::const_iterator ii;
  if (input.m_coordMap) {
    ii = input.m_coordMap->find(m_spaceID);
  } else {
    TRACE_ERR("No coordinates for space " << m_space << " on input (specify with coord XML tag)" << endl);
    TRACE_ERR("Scores for " << m_description << " will be incorrect and probably all zeros" << endl);
    broken = true;
  }
  if (ii == input.m_coordMap->end()) {
    TRACE_ERR("No coordinates for space " << m_space << " on input (specify with coord XML tag)" << endl);
    TRACE_ERR("Scores for " << m_description << " will be incorrect and probably all zeros" << endl);
    broken = true;
  }
  // Target phrase coord
  vector<SPTR<vector<float> > > const* tpp = targetPhrase.GetCoordList(m_spaceID);
  if (tpp == NULL) {
    TRACE_ERR("No coordinates for space " << m_space << " on target phrase (PhraseDictionary implementation needs to set)" << endl);
    TRACE_ERR("Scores for " << m_description << " will be incorrect and probably all zeros" << endl);
    broken = true;
  }
  // Compute scores
  if (!broken) {
    vector<float> const& inputCoord = ii->second;
    vector<SPTR<vector<float> > > const& tpCoord = *tpp;
    // Centroid of target phrase instances (from phrase extraction)
    vector<float> centroid = vector<float>(inputCoord.size(), 0);
    BOOST_FOREACH(SPTR<vector<float> > const coord, tpCoord) {
      for (size_t i = 0; i < inputCoord.size(); ++i) {
        centroid[i] += (*coord)[i];
      }
    }
    for (size_t i = 0; i < inputCoord.size(); ++i) {
      centroid[i] /= tpCoord.size();
    }
    // Average distance from the target phrase instances to (1) the input and
    // (2) the target phrase centroid
    float inputDistance = 0;
    float centroidDistance = 0;
    if (m_measure == EuclideanDistance) {
      BOOST_FOREACH(SPTR<vector<float> > const coord, tpCoord) {
        float pointInputDistance = 0;
        float pointCentroidDistance = 0;
        for (size_t i = 0; i < inputCoord.size(); ++i) {
          pointInputDistance += pow(inputCoord[i] - (*coord)[i], 2);
          pointCentroidDistance += pow(centroid[i] - (*coord)[i], 2);
        }
        inputDistance += sqrt(pointInputDistance);
        centroidDistance += sqrt(pointCentroidDistance);
      }
    } else if (m_measure == TotalVariationDistance) {
      BOOST_FOREACH(SPTR<vector<float> > const coord, tpCoord) {
        float pointInputDistance = 0;
        float pointCentroidDistance = 0;
        for (size_t i = 0; i < inputCoord.size(); ++i) {
          pointInputDistance += abs(inputCoord[i] - (*coord)[i]);
          pointCentroidDistance += abs(centroid[i] - (*coord)[i]);
        }
        inputDistance += pointInputDistance / 2;
        centroidDistance += pointCentroidDistance / 2;
      }
    }
    inputDistance /= tpCoord.size();
    centroidDistance /= tpCoord.size();
    // Log transform scores, max with float epsilon to avoid domain error
    scores[0] = log(max(inputDistance, Moses::FLOAT_EPSILON));
    scores[1] = log(max(centroidDistance, Moses::FLOAT_EPSILON));
  }
  // Set scores
  scoreBreakdown.Assign(this, scores);
  return;
}

void PhraseDistanceFeature::SetParameter(const string& key, const string& value)
{
  if (key == "space") {
    m_space = value;
    m_spaceID = StaticData::InstanceNonConst().MapCoordSpace(m_space);
  } else if (key == "measure") {
    if (value == "euc") {
      m_measure = EuclideanDistance;
    } else if (value == "var") {
      m_measure = TotalVariationDistance;
    } else {
      UTIL_THROW2("Unknown measure " << value << ", choices: euc var");
    }
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

} // namespace
