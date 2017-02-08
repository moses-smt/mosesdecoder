/*
 * Scores.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include <vector>
#include <cstddef>
#include <stdio.h>
#include "Scores.h"
#include "Weights.h"
#include "System.h"
#include "FF/FeatureFunction.h"
#include "FF/FeatureFunctions.h"
#include "legacy/Util2.h"

using namespace std;

namespace Moses2
{

Scores::Scores(const System &system, MemPool &pool, size_t numScores) :
  m_total(0)
{
  if (system.options.nbest.nbest_size) {
    m_scores = new (pool.Allocate<SCORE>(numScores)) SCORE[numScores];
    Init<SCORE>(m_scores, numScores, 0);
  } else {
    m_scores = NULL;
  }
}

Scores::Scores(const System &system, MemPool &pool, size_t numScores,
               const Scores &origScores) :
  m_total(origScores.m_total)
{
  if (system.options.nbest.nbest_size) {
    m_scores = new (pool.Allocate<SCORE>(numScores)) SCORE[numScores];
    memcpy(m_scores, origScores.m_scores, sizeof(SCORE) * numScores);
  } else {
    m_scores = NULL;
  }
}

Scores::~Scores()
{

}

const SCORE *Scores::GetScores(const FeatureFunction &featureFunction) const
{
  assert(m_scores);
  size_t ffStartInd = featureFunction.GetStartInd();
  const SCORE &scores = m_scores[ffStartInd];
  return &scores;
}

void Scores::Reset(const System &system)
{
  if (system.options.nbest.nbest_size) {
    size_t numScores = system.featureFunctions.GetNumScores();
    Init<SCORE>(m_scores, numScores, 0);
  }
  m_total = 0;
}

void Scores::PlusEquals(const System &system,
                        const FeatureFunction &featureFunction, const SCORE &score)
{
  assert(featureFunction.GetNumScores() == 1);

  const Weights &weights = system.weights;

  size_t ffStartInd = featureFunction.GetStartInd();
  if (system.options.nbest.nbest_size) {
    m_scores[ffStartInd] += score;
  }
  SCORE weight = weights[ffStartInd];
  m_total += score * weight;
}

void Scores::PlusEquals(const System &system,
                        const FeatureFunction &featureFunction, const SCORE &score, size_t offset)
{
  assert(offset < featureFunction.GetNumScores());

  const Weights &weights = system.weights;

  size_t ffStartInd = featureFunction.GetStartInd();
  if (system.options.nbest.nbest_size) {
    m_scores[ffStartInd + offset] += score;
  }
  SCORE weight = weights[ffStartInd + offset];
  m_total += score * weight;
}

void Scores::PlusEquals(const System &system,
                        const FeatureFunction &featureFunction, const std::vector<SCORE> &scores)
{
  assert(scores.size() == featureFunction.GetNumScores());

  const Weights &weights = system.weights;

  size_t ffStartInd = featureFunction.GetStartInd();
  for (size_t i = 0; i < scores.size(); ++i) {
    SCORE incrScore = scores[i];
    if (system.options.nbest.nbest_size) {
      m_scores[ffStartInd + i] += incrScore;
    }
    //cerr << "ffStartInd=" << ffStartInd << " " << i << endl;
    SCORE weight = weights[ffStartInd + i];
    m_total += incrScore * weight;
  }
}

void Scores::PlusEquals(const System &system,
                        const FeatureFunction &featureFunction, SCORE scores[])
{
  //assert(scores.size() == featureFunction.GetNumScores());

  const Weights &weights = system.weights;

  size_t ffStartInd = featureFunction.GetStartInd();
  for (size_t i = 0; i < featureFunction.GetNumScores(); ++i) {
    SCORE incrScore = scores[i];
    if (system.options.nbest.nbest_size) {
      m_scores[ffStartInd + i] += incrScore;
    }
    //cerr << "ffStartInd=" << ffStartInd << " " << i << endl;
    SCORE weight = weights[ffStartInd + i];
    m_total += incrScore * weight;
  }
}

void Scores::PlusEquals(const System &system, const Scores &other)
{
  size_t numScores = system.featureFunctions.GetNumScores();
  if (system.options.nbest.nbest_size) {
    for (size_t i = 0; i < numScores; ++i) {
      m_scores[i] += other.m_scores[i];
    }
  }
  m_total += other.m_total;
}

void Scores::MinusEquals(const System &system, const Scores &other)
{
  size_t numScores = system.featureFunctions.GetNumScores();
  if (system.options.nbest.nbest_size) {
    for (size_t i = 0; i < numScores; ++i) {
      m_scores[i] -= other.m_scores[i];
    }
  }
  m_total -= other.m_total;
}

void Scores::Assign(const System &system,
                    const FeatureFunction &featureFunction, const SCORE &score)
{
  assert(featureFunction.GetNumScores() == 1);

  const Weights &weights = system.weights;

  size_t ffStartInd = featureFunction.GetStartInd();

  if (system.options.nbest.nbest_size) {
    assert(m_scores[ffStartInd] == 0);
    m_scores[ffStartInd] = score;
  }
  SCORE weight = weights[ffStartInd];
  m_total += score * weight;

}

void Scores::Assign(const System &system,
                    const FeatureFunction &featureFunction, const std::vector<SCORE> &scores)
{
  assert(scores.size() == featureFunction.GetNumScores());

  const Weights &weights = system.weights;

  size_t ffStartInd = featureFunction.GetStartInd();
  for (size_t i = 0; i < scores.size(); ++i) {
    SCORE incrScore = scores[i];

    if (system.options.nbest.nbest_size) {
      assert(m_scores[ffStartInd + i] == 0);
      m_scores[ffStartInd + i] = incrScore;
    }
    //cerr << "ffStartInd=" << ffStartInd << " " << i << endl;
    SCORE weight = weights[ffStartInd + i];
    m_total += incrScore * weight;
  }
}

void Scores::CreateFromString(const std::string &str,
                              const FeatureFunction &featureFunction, const System &system,
                              bool transformScores)
{
  vector<SCORE> scores = Tokenize<SCORE>(str);
  if (transformScores) {
    std::transform(scores.begin(), scores.end(), scores.begin(),
                   TransformScore);
    std::transform(scores.begin(), scores.end(), scores.begin(), FloorScore);
  }

  /*
   std::copy(scores.begin(),scores.end(),
   std::ostream_iterator<SCORE>(cerr," "));
   */

  PlusEquals(system, featureFunction, scores);
}

std::string Scores::Debug(const System &system) const
{
  stringstream out;
  out << "total=" << m_total;

  if (system.options.nbest.nbest_size) {
    out << ", ";
    BOOST_FOREACH(const FeatureFunction *ff, system.featureFunctions.GetFeatureFunctions()) {
      out << ff->GetName() << "= ";
      for (size_t i = ff->GetStartInd(); i < (ff->GetStartInd() + ff->GetNumScores()); ++i) {
        out << m_scores[i] << " ";
      }
    }
  }

  return out.str();
}

void Scores::OutputBreakdown(std::ostream &out, const System &system) const
{
  if (system.options.nbest.nbest_size) {
    BOOST_FOREACH(const FeatureFunction *ff, system.featureFunctions.GetFeatureFunctions()) {
      if (ff->IsTuneable()) {
        out << ff->GetName() << "= ";
        for (size_t i = ff->GetStartInd(); i < (ff->GetStartInd() + ff->GetNumScores()); ++i) {
          out << m_scores[i] << " ";
        }
      }
    }
  }
}

// static functions to work out estimated scores
SCORE Scores::CalcWeightedScore(const System &system,
                                const FeatureFunction &featureFunction, SCORE scores[])
{
  SCORE ret = 0;

  const Weights &weights = system.weights;

  size_t ffStartInd = featureFunction.GetStartInd();
  for (size_t i = 0; i < featureFunction.GetNumScores(); ++i) {
    SCORE incrScore = scores[i];

    //cerr << "ffStartInd=" << ffStartInd << " " << i << endl;
    SCORE weight = weights[ffStartInd + i];
    ret += incrScore * weight;
  }

  return ret;
}

SCORE Scores::CalcWeightedScore(const System &system,
                                const FeatureFunction &featureFunction, SCORE score)
{
  const Weights &weights = system.weights;
  assert(featureFunction.GetNumScores() == 1);

  size_t ffStartInd = featureFunction.GetStartInd();
  SCORE weight = weights[ffStartInd];
  SCORE ret = score * weight;

  return ret;
}

}

