
#include <stdlib.h>
#include <algorithm>
#include "Scores.h"
#include "Global.h"
#include "Util.h"
#include "check.h"
#include "FF/FeatureFunction.h"

using namespace std;

Scores::Scores()
  :m_weightedScore(0)
#ifdef SCORE_BREAKDOWN
  ,m_scores(FeatureFunction::GetTotalNumScores(), 0)
#endif
{
}

Scores::Scores(const Scores &copy)
  :m_weightedScore(copy.m_weightedScore)
#ifdef SCORE_BREAKDOWN
  ,m_scores(copy.m_scores)
#endif
{
}

Scores::~Scores()
{
}

void Scores::CreateFromString(const FeatureFunction &ff, const std::string &line, bool logScores)
{
  std::vector<SCORE> scores(ff.GetNumScores());

  if (logScores) {
    std::vector<SCORE> probs(ff.GetNumScores());
    Tokenize<SCORE>(probs, line);
    std::transform(probs.begin(),probs.end(),scores.begin(),
                   TransformScore);
  } else {
    Tokenize<SCORE>(scores, line);
  }

  Add(ff, scores);
}

void Scores::Add(const Scores &other)
{
  m_weightedScore += other.m_weightedScore;

#ifdef SCORE_BREAKDOWN
  size_t numScores = FeatureFunction::GetTotalNumScores();
  for (size_t i = 0; i < numScores; ++i) {
    m_scores[i] += other.m_scores[i];
  }
#endif
}

void Scores::Add(const FeatureFunction &ff, SCORE score)
{
  size_t numScores = ff.GetNumScores();
  CHECK(numScores == 1);
  size_t startInd = ff.GetStartInd();

  // weighted score
  const Global &Global = Global::Instance();
  const std::vector<SCORE> &weights = Global.weights.GetWeights();
  SCORE weight = weights[startInd];

  m_weightedScore += weight * score;

  // update vector
#ifdef SCORE_BREAKDOWN
  m_scores[startInd] += score;
#endif
}

void Scores::Add(const FeatureFunction &ff, const std::vector<SCORE> &scores)
{
  size_t numScores = ff.GetNumScores();
  CHECK(numScores == scores.size());
  size_t startInd = ff.GetStartInd();

  const Global &Global = Global::Instance();
  const std::vector<SCORE> &weights = Global.weights.GetWeights();

  for (size_t i = 0; i < numScores; ++i) {
    size_t ffInd = startInd + i;
    SCORE score = scores[i];
    SCORE weight = weights[ffInd];

    // weighted score
    m_weightedScore += weight * score;

    // update vector
#ifdef SCORE_BREAKDOWN
    m_scores[ffInd] += score;
#endif
  }

}

std::string Scores::Debug() const
{
  stringstream strme;
  strme << "TOTAL=" << m_weightedScore;
#ifdef SCORE_BREAKDOWN
  strme << " [" << m_scores[0];

  size_t numScores = FeatureFunction::GetTotalNumScores();
  for (size_t i = 1; i < numScores; ++i) {
    strme << "," << m_scores[i];
  }
  strme << "]";
#endif
  return strme.str();

}
