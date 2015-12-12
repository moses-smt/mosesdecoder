// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include "OptionsBaseClass.h"

namespace Moses
{

  struct 
  ReportingOptions : public OptionsBaseClass
  {
    long start_translation_id;

    std::vector<FactorType> factor_order;
    std::string factor_delimiter;
    
    bool ReportAllFactors; // m_reportAllFactors;
    int ReportSegmentation; // 0: no 1: m_reportSegmentation 2: ..._enriched 

    bool PrintAlignmentInfo; // m_PrintAlignmentInfo
    bool PrintAllDerivations;
    bool PrintTranslationOptions; 

    WordAlignmentSort WA_SortOrder; // 0: no, 1: target order
    std::string AlignmentOutputFile; 
    
    bool WordGraph;

    std::string SearchGraph;
    std::string SearchGraphExtended;
    std::string SearchGraphSLF;
    std::string SearchGraphHG;
    std::string SearchGraphPB;
    bool DontPruneSearchGraph;

    bool RecoverPath; // recover input path?
    bool ReportHypoScore;

    bool PrintID;
    bool PrintPassThrough;

    // transrep = translation reporting
    std::string detailed_transrep_filepath;
    std::string detailed_tree_transrep_filepath;
    std::string detailed_all_transrep_filepath;
    bool include_lhs_in_search_graph;

    
    std::string lattice_sample_filepath; 
    size_t lattice_sample_size;

    bool init(Parameter const& param);

    /// do we need to keep the search graph from decoding?
    bool NeedSearchGraph() const {
      return !(SearchGraph.empty() && SearchGraphExtended.empty());
    }

#ifdef HAVE_XMLRPC_C
    bool update(std::map<std::string, xmlrpc_c::value>const& param);
#endif

    
    ReportingOptions();
  };
  
}

