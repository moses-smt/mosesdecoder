
#include "Weights.h"
#include "Util.h"
#include "check.h"
#include "FF/FeatureFunction.h"

using namespace std;

Weights::Weights()
  :m_weights(FeatureFunction::GetTotalNumScores(), 0)
{
  // TODO Auto-generated constructor stub

}

Weights::~Weights()
{
  // TODO Auto-generated destructor stub
}

void Weights::CreateFromString(const std::string &line)
{
  Tokenize<SCORE>(m_weights, line);

}

void Weights::SetWeights(const FeatureFunction &ff, const std::vector<SCORE> &weights)
{
  size_t numScores = ff.GetNumScores();
  CHECK(numScores == weights.size());
  size_t startInd = ff.GetStartInd();

  size_t inInd = 0;
  for (size_t i = startInd; i < startInd + numScores; ++i, ++inInd) {
    m_weights[i] = weights[inInd];
  }
}

