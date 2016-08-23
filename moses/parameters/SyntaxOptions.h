// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include "moses/Word.h"
#include <string>
#include "OptionsBaseClass.h"
#include "moses/FactorCollection.h"

namespace Moses
{
  typedef std::pair<std::string, float> UnknownLHSEntry;
  typedef std::vector<UnknownLHSEntry>  UnknownLHSList;

  struct 
  SyntaxOptions : public OptionsBaseClass
  {
    S2TParsingAlgorithm s2t_parsing_algo;
    Word input_default_non_terminal;
    Word output_default_non_terminal;
    bool default_non_term_only_for_empty_range; // whatever that means
    UnknownLHSList unknown_lhs;
    SourceLabelOverlap source_label_overlap; // m_sourceLabelOverlap;
    size_t rule_limit;

    SyntaxOptions();

    bool init(Parameter const& param);
    bool update(std::map<std::string,xmlrpc_c::value>const& param);
    void LoadNonTerminals(Parameter const& param, FactorCollection& factorCollection);
  };

}

