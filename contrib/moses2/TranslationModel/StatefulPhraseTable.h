#pragma once
#include "../FF/StatefulFeatureFunction.h"
#include "../Array.h"
//#include "../HypothesisColl.h"

namespace Moses2
{
class HypothesisBase;
typedef Array<const HypothesisBase*> Hypotheses;

class StatefulPhraseTable : public StatefulFeatureFunction
{
public:
  StatefulPhraseTable(size_t startInd, const std::string &line);

  virtual void BeforeExtending(const Hypotheses &hypos) const = 0;

};

}

