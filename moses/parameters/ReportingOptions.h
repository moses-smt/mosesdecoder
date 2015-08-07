// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
namespace Moses
{

  struct 
  ReportingOptions 
  {

    WordAlignmentSort sort_word_alignment; // 0: no, 1: target order

    
    bool segmentation; // m_reportSegmentation;
    bool segmentation_enriched; // m_reportSegmentationEnriched;
    bool all_factors; // m_reportAllFactors;

    bool output_word_graph;
    bool output_search_graph;
    bool output_search_graph_extended;
    bool output_search_graph_slf;
    bool output_search_graph_hypergraph;
    bool output_search_graph_protobuf;

    // print ..
    bool aln_info;    // m_PrintAlignmentInfo;
    bool id;          // m_PrintID;
    bool passthrough; // m_PrintPassthroughInformation;

    // transrep = translation reporting
    std::string detailed_transrep_filepath;
    std::string detailed_tree_transrep_filepath;
    std::string detailed_all_transrep_filepath;

    std::string aln_output_file; // m_alignmentOutputFile;

    bool init(Parameter const& param);
  };

}

