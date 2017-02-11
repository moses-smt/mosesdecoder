// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "InputOptions.h"
#include <vector>
#include <iostream>
#include "../legacy/Parameter.h"

namespace Moses2
{

InputOptions::
InputOptions()
  : continue_partial_translation(false)
  , input_type(SentenceInput)
  , xml_policy(XmlPassThrough)
  , placeholder_factor(NOT_FOUND)
{
  xml_brackets.first  = "<";
  xml_brackets.second = ">";
  factor_order.assign(1,0);
  factor_delimiter = "|";
}

bool
InputOptions::
init(Parameter const& param)
{
  param.SetParameter(input_type, "inputtype", SentenceInput);
#if 0
  if (input_type == SentenceInput) {
    VERBOSE(2, "input type is: text input");
  } else if (input_type == ConfusionNetworkInput) {
    VERBOSE(2, "input type is: confusion net");
  } else if (input_type == WordLatticeInput) {
    VERBOSE(2, "input type is: word lattice");
  } else if (input_type == TreeInputType) {
    VERBOSE(2, "input type is: tree");
  } else if (input_type == TabbedSentenceInput) {
    VERBOSE(2, "input type is: tabbed sentence");
  } else if (input_type == ForestInputType) {
    VERBOSE(2, "input type is: forest");
  }
#endif


  param.SetParameter(continue_partial_translation,
                     "continue-partial-translation", false);

  param.SetParameter<XmlInputType>(xml_policy, "xml-input", XmlPassThrough);

  // specify XML tags opening and closing brackets for XML option
  // Do we really want this to be configurable???? UG
  const PARAM_VEC *pspec;
  pspec = param.GetParam("xml-brackets");
  if (pspec && pspec->size()) {
    std::vector<std::string> brackets = Tokenize(pspec->at(0));
    if(brackets.size()!=2) {
      std::cerr << "invalid xml-brackets value, "
                << "must specify exactly 2 blank-delimited strings "
                << "for XML tags opening and closing brackets"
                << std::endl;
      exit(1);
    }

    xml_brackets.first= brackets[0];
    xml_brackets.second=brackets[1];

#if 0
    VERBOSE(1,"XML tags opening and closing brackets for XML input are: "
            << xml_brackets.first << " and "
            << xml_brackets.second << std::endl);
#endif
  }

  pspec = param.GetParam("input-factors");
  if (pspec) factor_order = Scan<FactorType>(*pspec);
  if (factor_order.empty()) factor_order.assign(1,0);
  param.SetParameter(placeholder_factor, "placeholder-factor", NOT_FOUND);

  param.SetParameter<std::string>(factor_delimiter, "factor-delimiter", "|");
  param.SetParameter<std::string>(input_file_path,"input-file","");

  return true;
}


#ifdef HAVE_XMLRPC_C
bool
InputOptions::
update(std::map<std::string,xmlrpc_c::value>const& param)
{
  typedef std::map<std::string, xmlrpc_c::value> params_t;
  params_t::const_iterator si = param.find("xml-input");
  if (si != param.end())
    xml_policy = Scan<XmlInputType>(xmlrpc_c::value_string(si->second));
  return true;
}
#endif

}
