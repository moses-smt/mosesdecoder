#include <iostream>

#include "SparseReorderingFeature.h"

using namespace std;

namespace Moses
{

SparseReorderingFeature::SparseReorderingFeature(const std::string &line)
  :StatefulFeatureFunction("StatefulFeatureFunction", line)
{
  cerr << "Constructing a Sparse Reordering feature" << endl;
}

FFState* SparseReorderingFeature::EvaluateChart(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new SparseReorderingState();
}


}

