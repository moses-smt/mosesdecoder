#pragma once

#include <string>
#include <map>
#include "StatelessFeatureFunction.h"
#include "FFState.h"
#include "moses/Factor.h"
#include "moses/TreeInput.h"

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

namespace Moses
{

// assumes that source-side syntax labels are stored in the target non-terminal field of the rules
class SourceGHKMTreeInputMatchFeature : public StatelessFeatureFunction
{
public:
  SourceGHKMTreeInputMatchFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const {};

  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;

  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const {};

  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const {};

};


}

