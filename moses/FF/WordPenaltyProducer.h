#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{
class TargetPhrase;
class ScoreComponentCollection;

/** Doesn't do anything but provide a key into the global
 *  score array to store the word penalty in.
 */
class WordPenaltyProducer : public StatelessFeatureFunction
{
public:
	WordPenaltyProducer(const std::string &line) : StatelessFeatureFunction("WordPenalty",1, line) {}

  virtual void Evaluate(const TargetPhrase &targetPhrase
                      , ScoreComponentCollection &scoreBreakdown
                      , ScoreComponentCollection &estimatedFutureScore) const;

};

}

