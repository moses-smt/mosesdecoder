#include "UnknownWordPenaltyProducer.h"
#include <vector>
#include <string>

using namespace std;

namespace Moses
{
UnknownWordPenaltyProducer::UnknownWordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction("UnknownWordPenalty",1, line)
{
  m_tuneable = false;
  ReadParameters();
}

}

