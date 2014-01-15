#include <vector>
#include <string>
#include "UnknownWordPenaltyProducer.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
UnknownWordPenaltyProducer *UnknownWordPenaltyProducer::s_instance = NULL;

UnknownWordPenaltyProducer::UnknownWordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction(1, line)
{
  m_tuneable = false;
  ReadParameters();

  UTIL_THROW_IF2(s_instance, "Can only have 1 unknown word penalty feature");
  s_instance = this;
}

std::vector<float> UnknownWordPenaltyProducer::DefaultWeights() const
{
  std::vector<float> ret(1, 1.0f);
  return ret;
}

}

