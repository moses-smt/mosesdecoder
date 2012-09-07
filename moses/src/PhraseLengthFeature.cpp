#include <sstream>
#include "PhraseLengthFeature.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"
#include "TranslationOption.h"

namespace Moses {

using namespace std;

void PhraseLengthFeature::Evaluate(
              const TranslationOption& translationOption,
              const InputType& inputType,
              const WordsBitmap& coverageVector,
              ScoreComponentCollection* accumulator) const
{
  // get length of source and target phrase
  size_t sourceLength = translationOption.GetTargetPhrase().GetSize();
  size_t targetLength = translationOption.GetSourcePhrase()->GetSize();

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
