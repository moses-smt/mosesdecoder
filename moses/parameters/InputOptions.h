// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include <string>
#include "OptionsBaseClass.h"

namespace Moses
{
  struct 
  InputOptions : public OptionsBaseClass
  {
    bool continue_partial_translation; 
    bool default_non_term_only_for_empty_range; // whatever that means
    InputTypeEnum input_type;
    XmlInputType  xml_policy; // pass through, ignore, exclusive, inclusive
    
    std::pair<std::string,std::string> xml_brackets; 
    // strings to use as XML tags' opening and closing brackets. 
    // Default are "<" and ">"

    bool init(Parameter const& param);
    InputOptions();
  };

}

