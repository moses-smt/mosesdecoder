#include "UnknownWordPenaltyProducer.h"
#include <vector>
#include <string>

using namespace std;

namespace Moses
{
UnknownWordPenaltyProducer::UnknownWordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction("UnknownWordPenalty",1, line) {
  m_tuneable = false;

  size_t ind = 0;
  while (ind < m_args.size()) {
	vector<string> &args = m_args[ind];
	bool consumed = SetParameter(args[0], args[1]);
	if (consumed) {
	  m_args.erase(m_args.begin() + ind);
	} else {
	  ++ind;
	}
  }
}

}

