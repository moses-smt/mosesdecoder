/*
 * SyntaxOptions.cpp
 *
 *  Created on: 13 Apr 2016
 *      Author: hieu
 */

#include "SyntaxOptions.h"
#include "../legacy/Parameter.h"

namespace Moses2
{
SyntaxOptions::SyntaxOptions()
  : s2t_parsing_algo(RecursiveCYKPlus)
  , default_non_term_only_for_empty_range(false)
  , source_label_overlap(SourceLabelOverlapAdd)
  , rule_limit(DEFAULT_MAX_TRANS_OPT_SIZE)
{}

bool SyntaxOptions::init(Parameter const& param)
{
  param.SetParameter(rule_limit, "rule-limit", DEFAULT_MAX_TRANS_OPT_SIZE);
  param.SetParameter(s2t_parsing_algo, "s2t-parsing-algorithm",
                     RecursiveCYKPlus);
  param.SetParameter(default_non_term_only_for_empty_range,
                     "default-non-term-for-empty-range-only", false);
  param.SetParameter(source_label_overlap, "source-label-overlap",
                     SourceLabelOverlapAdd);
  return true;
}

bool SyntaxOptions::update(std::map<std::string,xmlrpc_c::value>const& param)
{
  typedef std::map<std::string, xmlrpc_c::value> params_t;
  // params_t::const_iterator si = param.find("xml-input");
  // if (si != param.end())
  //   xml_policy = Scan<XmlInputType>(xmlrpc_c::value_string(si->second));
  return true;
}

void SyntaxOptions::LoadNonTerminals(Parameter const& param, FactorCollection& factorCollection)
{

}


} /* namespace Moses2 */
