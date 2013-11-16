#include "UnknownWordPenaltyProducer.h"
#include <vector>
#include <string>

using namespace std;

namespace Moses
{
UnknownWordPenaltyProducer::UnknownWordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction(1, line)
{
  m_tuneable = false;
  ReadParameters();
}

std::vector<float> UnknownWordPenaltyProducer::DefaultWeights() const
{
  std::vector<float> ret(1, 1.0f);
  return ret;
}

}

