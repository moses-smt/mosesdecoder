#pragma once
#include "../FF/StatefulFeatureFunction.h"
#include "../Array.h"
//#include "../HypothesisColl.h"

namespace Moses2
{
class HypothesisBase;
typedef Array<const HypothesisBase*> Hypotheses;
class Manager;

class StatefulPhraseTable : public StatefulFeatureFunction
{
public:
  StatefulPhraseTable(size_t startInd, const std::string &line);

  virtual void EvaluateBeforeExtending(const Hypotheses &hypos, const Manager &mgr) const = 0;

};

}

