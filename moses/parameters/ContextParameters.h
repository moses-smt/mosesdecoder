// -*- c++ -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"

namespace Moses
{

class ContextParameters
{
public:
  ContextParameters();
  void init(Parameter& params);
  size_t look_ahead;  // # of words to look ahead for context-sensitive decoding
  size_t look_back;   // # of works to look back for context-sensitive decoding
  std::string context_string; // fixed context string specified on command line
};

}
