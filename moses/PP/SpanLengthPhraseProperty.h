
#pragma once

#include "moses/PP/PhraseProperty.h"
#include <string>

namespace Moses
{

class SpanLengthPhraseProperty : public PhraseProperty
{
public:
	SpanLengthPhraseProperty(const std::string &value) :  PhraseProperty(value) {};

};

} // namespace Moses

