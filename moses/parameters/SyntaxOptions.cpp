// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "SyntaxOptions.h"
#include <vector>
#include <iostream>
#include "moses/StaticData.h"
#include "moses/TypeDef.h"
#include "moses/Factor.h"
#include "moses/InputFileStream.h"

namespace Moses {

  SyntaxOptions::
  SyntaxOptions()
    : s2t_parsing_algo(RecursiveCYKPlus)
    , default_non_term_only_for_empty_range(false)
    , source_label_overlap(SourceLabelOverlapAdd)
    , rule_limit(DEFAULT_MAX_TRANS_OPT_SIZE)
  { }

  bool
  SyntaxOptions::
  init(Parameter const& param)
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

  void
  SyntaxOptions::
  LoadNonTerminals(Parameter const& param, FactorCollection& factorCollection)
  {
    using namespace std;
    string dfltNonTerm;
    param.SetParameter<string>(dfltNonTerm, "non-terminals", "X");

    const Factor *srcFactor = factorCollection.AddFactor(Input, 0, dfltNonTerm, true);
    input_default_non_terminal.SetFactor(0, srcFactor);
    input_default_non_terminal.SetIsNonTerminal(true);

    const Factor *trgFactor = factorCollection.AddFactor(Output, 0, dfltNonTerm, true);
    output_default_non_terminal.SetFactor(0, trgFactor);
    output_default_non_terminal.SetIsNonTerminal(true);

    // for unknown words
    const PARAM_VEC *params = param.GetParam("unknown-lhs");
    if (params == NULL || params->size() == 0) {
      UnknownLHSEntry entry(dfltNonTerm, 0.0f);
      unknown_lhs.push_back(entry);
    } else {
      const string &filePath = params->at(0);
      InputFileStream inStream(filePath);
      string line;
      while(getline(inStream, line)) {
        vector<string> tokens = Tokenize(line);
        UTIL_THROW_IF2(tokens.size() != 2, "Incorrect unknown LHS format: " << line);
        UnknownLHSEntry entry(tokens[0], Scan<float>(tokens[1]));
        unknown_lhs.push_back(entry);
        factorCollection.AddFactor(Output, 0, tokens[0], true);
      }
    }
  }

#ifdef HAVE_XMLRPC_C
  bool 
  SyntaxOptions::
  update(std::map<std::string,xmlrpc_c::value>const& param)
  {
    typedef std::map<std::string, xmlrpc_c::value> params_t;
    // params_t::const_iterator si = param.find("xml-input");
    // if (si != param.end())
    //   xml_policy = Scan<XmlInputType>(xmlrpc_c::value_string(si->second));
    return true;
  }
#else
  bool 
  SyntaxOptions::
  update(std::map<std::string,xmlrpc_c::value>const& param)
  {}
#endif

}
