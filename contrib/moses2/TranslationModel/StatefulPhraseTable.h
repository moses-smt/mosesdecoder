#pragma once
#include "../FF/StatefulFeatureFunction.h"


namespace Moses2
{
class StatefulPhraseTable : public StatefulFeatureFunction
{
public:
  StatefulPhraseTable(size_t startInd, const std::string &line);

};

}

