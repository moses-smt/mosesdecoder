#include <sstream>
#include "PhraseLengthFeature.h"
#include "moses/Hypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TranslationOption.h"

namespace Moses
{

using namespace std;

PhraseLengthFeature::PhraseLengthFeature(const std::string &line)
  :StatelessFeatureFunction("PhraseLengthFeature", 0, line)
{
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

void PhraseLengthFeature::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  // get length of source and target phrase
  size_t targetLength = targetPhrase.GetSize();
  size_t sourceLength = source.GetSize();

  // create feature names
  stringstream nameSource;
  nameSource << "s" << sourceLength;

  stringstream nameTarget;
  nameTarget << "t" << targetLength;

  stringstream nameBoth;
  nameBoth << sourceLength << "," << targetLength;

  // increase feature counts
  scoreBreakdown.PlusEquals(this,nameSource.str(),1);
  scoreBreakdown.PlusEquals(this,nameTarget.str(),1);
  scoreBreakdown.PlusEquals(this,nameBoth.str(),1);

  //cerr << nameSource.str() << " " << nameTarget.str() << " " << nameBoth.str() << endl;
}

}
