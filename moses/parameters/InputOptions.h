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
    InputTypeEnum input_type;
    XmlInputType  xml_policy; // pass through, ignore, exclusive, inclusive
    std::vector<FactorType> factor_order; // input factor order
    std::string factor_delimiter; 
    FactorType placeholder_factor; // where to store original text for placeholders 
    std::string input_file_path;
    std::pair<std::string,std::string> xml_brackets; 
    // strings to use as XML tags' opening and closing brackets. 
    // Default are "<" and ">"

    InputOptions();

    bool init(Parameter const& param);
    bool update(std::map<std::string,xmlrpc_c::value>const& param);

  };

}

