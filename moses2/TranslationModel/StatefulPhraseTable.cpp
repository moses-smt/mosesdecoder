#include "StatefulPhraseTable.h"

namespace Moses2
{

StatefulPhraseTable::StatefulPhraseTable(size_t startInd,
    const std::string &line) :
  StatefulFeatureFunction(startInd, line)
{
}

}

