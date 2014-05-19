
#pragma once

#include "moses/PP/PhraseProperty.h"
#include <string>

namespace Moses
{

class TreeStructurePhraseProperty : public PhraseProperty
{
public:
  TreeStructurePhraseProperty(const std::string &value) :  PhraseProperty(value) {};

};

} // namespace Moses

