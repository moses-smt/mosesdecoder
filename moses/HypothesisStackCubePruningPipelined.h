#ifndef moses_HypothesisStackCubePruningPipelined_h
#define moses_HypothesisStackCubePruningPipelined_h

#include "moses/HypothesisStackCubePruning.h"
#include "moses/PipelinedLM.h"

namespace Moses
{

class HypothesisStackCubePruningPipelined : public HypothesisStackCubePruning
  {
    public:
      HypothesisStackCubePruningPipelined(Manager& manager, PipelinedLM& pipeline0, PipelinedLM& pipeline1);
      bool AddPrune(Hypothesis* hypo);
      void Drain();
      void AddScored(Hypothesis* hypo);
      void SetupPipelines() {
        m_pipelinedLM0.SetStack(this);
        m_pipelinedLM1.SetStack(this);
      }

    private:
      PipelinedLM& m_pipelinedLM0;
      PipelinedLM& m_pipelinedLM1;
  };
}
#endif
