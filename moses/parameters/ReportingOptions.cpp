// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "ReportingOptions.h"
#include "moses/Parameter.h"

namespace Moses {
  using namespace std;
  bool
  ReportingOptions::
  init(Parameter const& param)
  {
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
    } else {
      lattice_sample_size = 0;
    }

    params= param.GetParam("output-factors");
    if (params) factor_order = Scan<FactorType>(*params);
    if (factor_order.empty()) factor_order.assign(1,0);

    return true;
  }

#ifdef HAVE_XMLRPC_C
  bool 
  ReportingOptions::
  update(std::map<std::string, xmlrpc_c::value>const& param)
  {
    ReportAllFactors = check(param, "report-all-factors", ReportAllFactors);
    return true;
  }
#endif

}
