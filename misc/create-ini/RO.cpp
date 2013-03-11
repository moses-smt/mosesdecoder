#include "RO.h"

int RO::s_index = 0;

RO::RO(const std::string &line, const std::pair<Factors, Factors> *factors)
:FF(line)
{
  index = s_index++;
  name = "LexicalReordering";
  numFeatures = 6;
  path = toks[0];
  type = "wbe-msd-bidirectional-fe-allff"; // TODO what is this?
  fileTypeSuffix = "wbe-msd-bidirectional-fe";
  
  if (factors) {
    inFactors = factors->first;
    outFactors = factors->second;
  }
}

