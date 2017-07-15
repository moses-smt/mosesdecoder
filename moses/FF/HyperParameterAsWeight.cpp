#include "HyperParameterAsWeight.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{

HyperParameterAsWeight::HyperParameterAsWeight(const std::string &line)
  :StatelessFeatureFunction(2, line)
{
  ReadParameters();

  // hack into StaticData and change anything you want
  // as an example, we have 2 weights and change
  //   1. stack size
  //   2. beam width
  StaticData &staticData = StaticData::InstanceNonConst();

  vector<float> weights = staticData.GetWeights(this);

  staticData.m_options->search.stack_size = weights[0] * 1000;
  staticData.m_options->search.beam_width = weights[1] * 10;

}


}

