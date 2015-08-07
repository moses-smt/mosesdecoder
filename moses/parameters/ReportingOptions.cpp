// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#if 0
#include "ReportingOptions.h"
#include "moses/Parameter.h"

namespace Moses {
  using namespace std;
  bool
  ReportingOptions::
  init(Parameter const& param)
  {
    PARAM_VEC const* params;

    param.SetParameter(segmentation, "report-segmentation", false );
    param.SetParameter(segmentation_enriched, "report-segmentation-enriched", false);
    param.SetParameter(all_factors, "report-all-factors", false );

    // print ...
    param.SetParameter(id, "print-id", false );
    param.SetParameter(aln_info, "print-alignment-info", false);
    param.SetParameter(passthrough, "print-passthrough", false );

    param.SetParameter<string>(detailed_transrep_filepath, "translation-details", "");
    param.SetParameter<string>(detailed_tree_transrep_filepath, 
			       "tree-translation-details", "");
    param.SetParameter<string>(detailed_all_transrep_filepath, 
			       "translation-all-details", "");

    // output search graph
    param.SetParameter<string>(output, 
			       "translation-all-details", "");



    param.SetParameter(sort_word_alignment, "sort-word-alignment", NoSort);


    // Is there a reason why we can't use SetParameter here? [UG]
     = param.GetParam("alignment-output-file");
    if (params && params->size()) {
      m_alignmentOutputFile = Scan<std::string>(params->at(0));
    }
    
    params = param.GetParam("output-word-graph");
    output_word_graph = (params && params->size() == 2);

    // bizarre code ahead! Why do we need to do the checks here?
    // as adapted from StaticData.cpp 
    params = param.GetParam("output-search-graph");
    if (params && params->size()) {
      if (params->size() != 1) {
	std::cerr << "ERROR: wrong format for switch -output-search-graph file";
	return false;
      }
      output_search_graph = true;
    }
    else if (m_parameter->GetParam("output-search-graph-extended") &&
	     m_parameter->GetParam("output-search-graph-extended")->size()) {
      if (m_parameter->GetParam("output-search-graph-extended")->size() != 1) {
	std::cerr << "ERROR: wrong format for switch -output-search-graph-extended file";
	return false;
      }
      output_search_graph = true;
      m_outputSearchGraphExtended = true;
    } else {
      m_outputSearchGraph = false;
    }
    
    params = m_parameter->GetParam("output-search-graph-slf");
    output_search_graph_slf = params && params->size();
    params = m_parameter->GetParam("output-search-graph-hypergraph");
    output_search_graph_hypergraph = params && params->size();

#ifdef HAVE_PROTOBUF
  params = m_parameter->GetParam("output-search-graph-pb");
  if (params && params->size()) {
    if (params->size() != 1) {
      cerr << "ERROR: wrong format for switch -output-search-graph-pb path";
      return false;
    }
    m_outputSearchGraphPB = true;
  } else
    m_outputSearchGraphPB = false;
#endif


    return true;
  }
}
#endif
