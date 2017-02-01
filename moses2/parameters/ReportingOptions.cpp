// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "ReportingOptions.h"
#include "../legacy/Parameter.h"

namespace Moses2
{
using namespace std;

ReportingOptions::
ReportingOptions()
  : start_translation_id(0)
  , ReportAllFactors(false)
  , ReportSegmentation(0)
  , PrintAlignmentInfo(false)
  , PrintAllDerivations(false)
  , PrintTranslationOptions(false)
  , WA_SortOrder(NoSort)
  , WordGraph(false)
  , DontPruneSearchGraph(false)
  , RecoverPath(false)
  , ReportHypoScore(false)
  , PrintID(false)
  , PrintPassThrough(false)
  , include_lhs_in_search_graph(false)
  , lattice_sample_size(0)
{
  factor_order.assign(1,0);
  factor_delimiter = "|";
}

bool
ReportingOptions::
init(Parameter const& param)
{
  param.SetParameter<long>(start_translation_id, "start-translation-id", 0);

  // including factors in the output
  param.SetParameter(ReportAllFactors, "report-all-factors", false);

  // segmentation reporting
  ReportSegmentation = (param.GetParam("report-segmentation-enriched")
                        ? 2 : param.GetParam("report-segmentation")
                        ? 1 : 0);

  // word alignment reporting
  param.SetParameter(PrintAlignmentInfo, "print-alignment-info", false);
  param.SetParameter(WA_SortOrder, "sort-word-alignment", NoSort);
  std::string e; // hack to save us param.SetParameter<string>(...)
  param.SetParameter(AlignmentOutputFile,"alignment-output-file", e);


  param.SetParameter(PrintAllDerivations, "print-all-derivations", false);
  param.SetParameter(PrintTranslationOptions, "print-translation-option", false);

  // output a word graph
  PARAM_VEC const* params;
  params = param.GetParam("output-word-graph");
  WordGraph = (params && params->size() == 2); // what are the two options?

  // dump the search graph
  param.SetParameter(SearchGraph, "output-search-graph", e);
  param.SetParameter(SearchGraphExtended, "output-search-graph-extended", e);
  param.SetParameter(SearchGraphSLF,"output-search-graph-slf", e);
  param.SetParameter(SearchGraphHG, "output-search-graph-hypergraph", e);
#ifdef HAVE_PROTOBUF
  param.SetParameter(SearchGraphPB, "output-search-graph-pb", e);
#endif

  param.SetParameter(DontPruneSearchGraph, "unpruned-search-graph", false);
  param.SetParameter(include_lhs_in_search_graph,
                     "include-lhs-in-search-graph", false );


  // miscellaneous
  param.SetParameter(RecoverPath, "recover-input-path",false);
  param.SetParameter(ReportHypoScore, "output-hypo-score",false);
  param.SetParameter(PrintID, "print-id",false);
  param.SetParameter(PrintPassThrough, "print-passthrough",false);
  param.SetParameter(detailed_all_transrep_filepath,
                     "translation-all-details", e);
  param.SetParameter(detailed_transrep_filepath, "translation-details", e);
  param.SetParameter(detailed_tree_transrep_filepath,
                     "tree-translation-details", e);

  params = param.GetParam("lattice-samples");
  if (params) {
    if (params->size() ==2 ) {
      lattice_sample_filepath = params->at(0);
      lattice_sample_size = Scan<size_t>(params->at(1));
    } else {
      std::cerr <<"wrong format for switch -lattice-samples file size";
      return false;
    }
  }


  if (ReportAllFactors) {
    factor_order.clear();
    for (size_t i = 0; i < MAX_NUM_FACTORS; ++i)
      factor_order.push_back(i);
  } else {
    params= param.GetParam("output-factors");
    if (params) factor_order = Scan<FactorType>(*params);
    if (factor_order.empty()) factor_order.assign(1,0);
  }

  param.SetParameter(factor_delimiter, "factor-delimiter", std::string("|"));
  param.SetParameter(factor_delimiter, "output-factor-delimiter", factor_delimiter);

  return true;
}

#ifdef HAVE_XMLRPC_C
bool
ReportingOptions::
update(std::map<std::string, xmlrpc_c::value>const& param)
{
  ReportAllFactors = check(param, "report-all-factors", ReportAllFactors);


  std::map<std::string, xmlrpc_c::value>::const_iterator m;
  m = param.find("output-factors");
  if (m  != param.end()) {
    factor_order=Tokenize<FactorType>(xmlrpc_c::value_string(m->second),",");
  }

  if (ReportAllFactors) {
    factor_order.clear();
    for (size_t i = 0; i < MAX_NUM_FACTORS; ++i)
      factor_order.push_back(i);
  }

  m = param.find("align");
  if (m != param.end() && Scan<bool>(xmlrpc_c::value_string(m->second)))
    ReportSegmentation = 1;

  PrintAlignmentInfo = check(param,"word-align",PrintAlignmentInfo);

  m = param.find("factor-delimiter");
  if (m != param.end()) {
    factor_delimiter = Trim(xmlrpc_c::value_string(m->second));
  }

  m = param.find("output-factor-delimiter");
  if (m != param.end()) {
    factor_delimiter = Trim(xmlrpc_c::value_string(m->second));
  }

  return true;
}
#endif
}
