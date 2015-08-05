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

    // transformation to log of a few scores
    beam_width = TransformScore(beam_width);
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
