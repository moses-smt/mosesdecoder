// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "OptionsBaseClass.h"

namespace Moses2
{

class ContextParameters : public OptionsBaseClass
{
public:
  ContextParameters();
  bool init(Parameter const& params);
  size_t look_ahead;  // # of words to look ahead for context-sensitive decoding
  size_t look_back;   // # of works to look back for context-sensitive decoding
  std::string context_string; // fixed context string specified on command line
};

}
