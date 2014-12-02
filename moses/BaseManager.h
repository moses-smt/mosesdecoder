#pragma once

#include <iostream>
#include <string>
#include "ScoreComponentCollection.h"

namespace Moses
{
class ScoreComponentCollection;
class FeatureFunction;

class BaseManager
{
protected:
  void OutputAllFeatureScores(const Moses::ScoreComponentCollection &features
							  , std::ostream &out);
  void OutputFeatureScores( std::ostream& out
							, const ScoreComponentCollection &features
							, const FeatureFunction *ff
							, std::string &lastName );

};

}
