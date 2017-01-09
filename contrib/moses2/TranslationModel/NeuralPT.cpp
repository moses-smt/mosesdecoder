#include "NeuralPT.h"

namespace Moses2
{
NeuralPT::NeuralPT(size_t startInd, const std::string &line)
:StatefulPhraseTable(startInd, line)
{
  ReadParameters();

}

FFState* NeuralPT::BlankState(MemPool &pool, const System &sys) const
{

}

void NeuralPT::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
    const InputType &input, const Hypothesis &hypo) const
{

}

void NeuralPT::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{

}

void NeuralPT::EvaluateWhenApplied(const SCFG::Manager &mgr,
    const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
    FFState &state) const
{

}


}
