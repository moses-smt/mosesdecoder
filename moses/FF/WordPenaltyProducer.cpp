#include "WordPenaltyProducer.h"
#include "moses/TargetPhrase.h"
#include "moses/ScoreComponentCollection.h"

using namespace std;

namespace Moses
{
WordPenaltyProducer::WordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction("WordPenalty",1, line) {
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

void WordPenaltyProducer::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  float score = - (float) targetPhrase.GetNumTerminals();
  scoreBreakdown.Assign(this, score);
}

}

