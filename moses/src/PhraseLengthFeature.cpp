#include <sstream>
#include "PhraseLengthFeature.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"

namespace Moses {

using namespace std;

void PhraseLengthFeature::Evaluate(const TargetPhrase& targetPhrase,
                                   ScoreComponentCollection* accumulator) const
{
  // get length of source and target phrase
  size_t sourceLength = targetPhrase.GetSourcePhrase().GetSize();
  size_t targetLength = targetPhrase.GetSize();

  // create feature names
  stringstream nameSource;
  nameSource << "s" << sourceLength;

  stringstream nameTarget;
  nameTarget << "t" << targetLength;

  stringstream nameBoth;
  nameBoth << sourceLength << "," << targetLength;

  // increase feature counts
  accumulator->PlusEquals(this,nameSource.str(),1);
  accumulator->PlusEquals(this,nameTarget.str(),1);
  accumulator->PlusEquals(this,nameBoth.str(),1);

  //cerr << nameSource.str() << " " << nameTarget.str() << " " << nameBoth.str() << endl;
}

}
