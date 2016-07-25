#ifndef moses_HypothesisStackCubePruningPipelined_h
#define moses_HypothesisStackCubePruningPipelined_h

#include "moses/HypothesisStackCubePruning.h"
#include "moses/PipelinedLM.h"

namespace Moses
{

class HypothesisStackCubePruningPipelined : public HypothesisStackCubePruning
  {
    public:
      HypothesisStackCubePruningPipelined(Manager& manager);
      bool AddPrune(Hypothesis* hypo);
      void Drain();
      void AddScored(Hypothesis* hypo);

    private:
      PipelinedLM m_pipelinedLM0;
      PipelinedLM m_pipelinedLM1;
  };
}
#endif
