// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "SearchOptions.h"

namespace Moses
{
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
    param.SetParameter(max_phrase_length, "max-phrase-length", 
                       DEFAULT_MAX_PHRASE_LENGTH);
    param.SetParameter(trans_opt_threshold, "translation-option-threshold", 
                       DEFAULT_TRANSLATION_OPTION_THRESHOLD);
    param.SetParameter(max_trans_opt_per_cov, "max-trans-opt-per-coverage", 
                       DEFAULT_MAX_TRANS_OPT_SIZE);
    param.SetParameter(max_partial_trans_opt, "max-partial-trans-opt", 
                       DEFAULT_MAX_PART_TRANS_OPT_SIZE);


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


}
