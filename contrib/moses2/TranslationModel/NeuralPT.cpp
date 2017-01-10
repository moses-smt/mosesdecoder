#include <boost/foreach.hpp>
#include "NeuralPT.h"
#include "../PhraseBased/Hypothesis.h"

namespace Moses2
{
class NeuralPTState: public FFState
{
public:

  virtual size_t hash() const
  { return 0; }
  virtual bool operator==(const FFState& o) const
  { return true; }

  virtual std::string ToString() const
  {
    return "NeuralPTState";
  }
};


////////////////////////////////////////////////////////////////////////////////////////
NeuralPT::NeuralPT(size_t startInd, const std::string &line)
:StatefulPhraseTable(startInd, line)
{
  ReadParameters();

}

FFState* NeuralPT::BlankState(MemPool &pool, const System &sys) const
{
  return new NeuralPTState();
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

void NeuralPT::BeforeExtending(const Hypotheses &hypos, const Manager &mgr) const
{
  BOOST_FOREACH(const HypothesisBase *hypo, hypos) {
    HypothesisBase *h1 = const_cast<HypothesisBase*>(hypo);
    Hypothesis &h2 = *static_cast<Hypothesis*>(h1);
    BeforeExtending(h2, mgr);
  }

}

void NeuralPT::BeforeExtending(Hypothesis &hypo, const Manager &mgr) const
{
  FFState *state = hypo.GetState(GetStartInd());
  NeuralPTState *stateCast = static_cast<NeuralPTState*>(state);

  std::vector<NeuralPhrase> tps = Lookup(*stateCast);
  BOOST_FOREACH(const NeuralPhrase &tp, tps) {

  }
}

std::vector<NeuralPhrase> NeuralPT::Lookup(const NeuralPTState &state) const
{
  std::vector<NeuralPhrase> ret;

  return ret;
}

}
