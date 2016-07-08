#ifndef moses_HypothesisStackCubePruningPipelined_h
#define moses_HypothesisStackCubePruningPipelined_h

#include "HypothesisStackCubePruning.h"
#include "FF/StatefulFeatureFunction.h"
#include "LM/Ken.h"
#include "lm/model.hh"
#include <vector>
namespace Moses
{
  class HypothesisStackCubePruningPipelined : public HypothesisStackCubePruning 
  {
    typedef lm::ngram::detail::HashedSearch<lm::ngram::BackoffValue> Search;
    Search& ExtractSearch();

    public:
      HypothesisStackCubePruningPipelined(Manager& manager) : HypothesisStackCubePruning(manager){
        //Extract model
        ExtractSearch();
      }

      bool AddPrune(Hypothesis* hypo);
  };
}
#endif
