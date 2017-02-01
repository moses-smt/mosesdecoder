/*
 * SyntaxOptions.h
 *
 *  Created on: 13 Apr 2016
 *      Author: hieu
 */
#pragma once
#include <string>
#include <vector>
#include "OptionsBaseClass.h"
#include "../SCFG/Word.h"

namespace Moses2
{
class FactorCollection;
class Parameter;

typedef std::pair<std::string, float> UnknownLHSEntry;
typedef std::vector<UnknownLHSEntry>  UnknownLHSList;

struct
    SyntaxOptions : public OptionsBaseClass {
  S2TParsingAlgorithm s2t_parsing_algo;
  SCFG::Word input_default_non_terminal;
  SCFG::Word output_default_non_terminal;
  bool default_non_term_only_for_empty_range; // whatever that means
  UnknownLHSList unknown_lhs;
  SourceLabelOverlap source_label_overlap; // m_sourceLabelOverlap;
  size_t rule_limit;

  SyntaxOptions();

  bool init(Parameter const& param);
  bool update(std::map<std::string,xmlrpc_c::value>const& param);
  void LoadNonTerminals(Parameter const& param, FactorCollection& factorCollection);
};

} /* namespace Moses2 */

