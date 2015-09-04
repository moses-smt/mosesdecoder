// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "InputOptions.h"
#include <vector>
#include <iostream>
#include "moses/StaticData.h"

namespace Moses {

  InputOptions::
  InputOptions()
  { 
    xml_brackets.first  = "<";
    xml_brackets.second = ">";
    input_type = SentenceInput;
  }

  bool
  InputOptions::
  init(Parameter const& param)
  {
    param.SetParameter(input_type, "inputtype", SentenceInput);
    if (input_type == SentenceInput) 
      { VERBOSE(2, "input type is: text input"); }
    else if (input_type == ConfusionNetworkInput)
      { VERBOSE(2, "input type is: confusion net"); }
    else if (input_type == WordLatticeInput)
      { VERBOSE(2, "input type is: word lattice"); }
    else if (input_type == TreeInputType)
      { VERBOSE(2, "input type is: tree"); }
    else if (input_type == TabbedSentenceInput)
      { VERBOSE(2, "input type is: tabbed sentence"); }
    else if (input_type == ForestInputType)
      { VERBOSE(2, "input type is: forest"); }

    param.SetParameter(continue_partial_translation, 
		       "continue-partial-translation", false);
    param.SetParameter(default_non_term_only_for_empty_range,
		       "default-non-term-for-empty-range-only", false);

    param.SetParameter<XmlInputType>(xml_policy, "xml-input", XmlPassThrough);
    
    // specify XML tags opening and closing brackets for XML option
    // Do we really want this to be configurable???? UG
    const PARAM_VEC *pspec;
    pspec = param.GetParam("xml-brackets");
    if (pspec && pspec->size()) 
      {
	std::vector<std::string> brackets = Tokenize(pspec->at(0));
	if(brackets.size()!=2) 
	  {
	    std::cerr << "invalid xml-brackets value, "
		      << "must specify exactly 2 blank-delimited strings "
		      << "for XML tags opening and closing brackets" << std::endl;
	    exit(1);
	  }
	xml_brackets.first= brackets[0];
	xml_brackets.second=brackets[1];
	VERBOSE(1,"XML tags opening and closing brackets for XML input are: "
		<< xml_brackets.first << " and " 
		<< xml_brackets.second << std::endl);
      }
    return true;
  }

}
