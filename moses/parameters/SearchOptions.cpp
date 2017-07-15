// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "SearchOptions.h"

namespace Moses
{

  SearchOptions::
  SearchOptions()
    : algo(Normal)
    , stack_size(DEFAULT_MAX_HYPOSTACK_SIZE)
    , stack_diversity(0)
    , disable_discarding(false)
    , max_phrase_length(DEFAULT_MAX_PHRASE_LENGTH)
    , max_trans_opt_per_cov(DEFAULT_MAX_TRANS_OPT_SIZE)
    , max_partial_trans_opt(DEFAULT_MAX_PART_TRANS_OPT_SIZE)
    , beam_width(DEFAULT_BEAM_WIDTH)
    , timeout(0)
    , consensus(false)
    , early_discarding_threshold(DEFAULT_EARLY_DISCARDING_THRESHOLD)
    , trans_opt_threshold(DEFAULT_TRANSLATION_OPTION_THRESHOLD)
  { }

  SearchOptions::
  SearchOptions(Parameter const& param)
    : stack_diversity(0)
  {
    init(param);
  }

  bool
  SearchOptions::
  init(Parameter const& param)
  {
    param.SetParameter(algo, "search-algorithm", Normal);
    param.SetParameter(stack_size, "stack", DEFAULT_MAX_HYPOSTACK_SIZE);
    param.SetParameter(stack_diversity, "stack-diversity", size_t(0));
    param.SetParameter(beam_width, "beam-threshold", DEFAULT_BEAM_WIDTH);
    param.SetParameter(early_discarding_threshold, "early-discarding-threshold", 
                       DEFAULT_EARLY_DISCARDING_THRESHOLD);
    param.SetParameter(timeout, "time-out", 0);
    param.SetParameter(segment_timeout, "segment-time-out", 0);
    param.SetParameter(max_phrase_length, "max-phrase-length", 
                       DEFAULT_MAX_PHRASE_LENGTH);
    param.SetParameter(trans_opt_threshold, "translation-option-threshold", 
                       DEFAULT_TRANSLATION_OPTION_THRESHOLD);
    param.SetParameter(max_trans_opt_per_cov, "max-trans-opt-per-coverage", 
                       DEFAULT_MAX_TRANS_OPT_SIZE);
    param.SetParameter(max_partial_trans_opt, "max-partial-trans-opt", 
                       DEFAULT_MAX_PART_TRANS_OPT_SIZE);

    param.SetParameter(consensus, "consensus-decoding", false);
    param.SetParameter(disable_discarding, "disable-discarding", false);
    
    // transformation to log of a few scores
    beam_width = TransformScore(beam_width);
    trans_opt_threshold = TransformScore(trans_opt_threshold);
    early_discarding_threshold = TransformScore(early_discarding_threshold);

    return true;
  }

  bool
  is_syntax(SearchAlgorithm algo) 
  {
    return (algo == CYKPlus   || algo == ChartIncremental ||
            algo == SyntaxS2T || algo == SyntaxT2S ||
            algo == SyntaxF2S || algo == SyntaxT2S_SCFG);
  }

#ifdef HAVE_XMLRPC_C
    bool 
    SearchOptions::
    update(std::map<std::string,xmlrpc_c::value>const& params)
    {
      typedef std::map<std::string, xmlrpc_c::value> params_t;

      params_t::const_iterator si = params.find("search-algorithm");
      if (si != params.end()) 
        {
          // use named parameters
          std::string spec = xmlrpc_c::value_string(si->second);
          if      (spec == "normal" || spec == "0") algo = Normal;
          else if (spec == "cube"   || spec == "1") algo = CubePruning;
          else throw xmlrpc_c::fault("Unsupported search algorithm", 
                                     xmlrpc_c::fault::CODE_PARSE);
        }

      si = params.find("stack");
      if (si != params.end()) stack_size = xmlrpc_c::value_int(si->second);

      si = params.find("stack-diversity");
      if (si != params.end()) stack_diversity = xmlrpc_c::value_int(si->second);

      si = params.find("beam-threshold");
      if (si != params.end()) beam_width = xmlrpc_c::value_double(si->second);

      si = params.find("time-out");
      if (si != params.end()) timeout = xmlrpc_c::value_int(si->second);
      
      si = params.find("max-phrase-length");
      if (si != params.end()) max_phrase_length = xmlrpc_c::value_int(si->second);
      
      return true;
    }
#else
    bool 
    SearchOptions::
    update(std::map<std::string,xmlrpc_c::value>const& params)
    {}
#endif

}
