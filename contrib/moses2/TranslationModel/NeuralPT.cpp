#include <boost/foreach.hpp>
#include "NeuralPT.h"
#include "../PhraseBased/Hypothesis.h"
#include "plugin/nmt.h"

using namespace std;

namespace Moses2
{
class NeuralPTState: public FFState
{
public:
  size_t lastWord;

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
, m_batchSize(1000)
, m_stateLength(5)
, m_factor(0)
, m_maxDevices(1)
, m_filteredSoftmax(0)
, m_mode("precalculate")
, m_threadId(0)
{
  ReadParameters();

}

void NeuralPT::Load(System &system)
{
  m_plugin = new amunmt::MosesPlugin();
  m_plugin->initGod(m_modelPath);

  size_t devices = amunmt::MosesPlugin::GetDevices(m_maxDevices);
  std::cerr << devices << std::endl;

  //m_sourceVocab = NMT::NewVocab(m_sourceVocabPath);
  //m_targetVocab = NMT::NewVocab(m_targetVocabPath);

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

void NeuralPT::EvaluateBeforeExtending(const Hypotheses &hypos, const Manager &mgr) const
{
  BOOST_FOREACH(const HypothesisBase *hypo, hypos) {
    HypothesisBase *h1 = const_cast<HypothesisBase*>(hypo);
    Hypothesis &h2 = *static_cast<Hypothesis*>(h1);
    EvaluateBeforeExtending(h2, mgr);
  }

}

void NeuralPT::EvaluateBeforeExtending(Hypothesis &hypo, const Manager &mgr) const
{
  FFState *state = hypo.GetState(GetStartInd());
  NeuralPTState *stateCast = static_cast<NeuralPTState*>(state);

  std::vector<AmunPhrase> tps = Lookup(*stateCast);
  BOOST_FOREACH(const AmunPhrase &tp, tps) {
    // TODO merge with existing pt
  }
}

std::vector<AmunPhrase> NeuralPT::Lookup(const NeuralPTState &prevState) const
{
  std::vector<AmunPhrase> ret;
  // TODO get phrases from NMT
  //prevState

  return ret;
}

void NeuralPT::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "state-length") {
    m_stateLength = Scan<size_t>(value);
  } else if (key == "model") {
    m_modelPath = value;
  } else if (key == "filtered-softmax") {
    m_filteredSoftmax = Scan<size_t>(value);
  } else if (key == "mode") {
    m_mode = value;
  } else if (key == "devices") {
    m_maxDevices = Scan<size_t>(value);
  } else if (key == "batch-size") {
    m_batchSize = Scan<size_t>(value);
  } else if (key == "source-vocab") {
    m_sourceVocabPath = value;
  } else if (key == "target-vocab") {
    m_targetVocabPath = value;
  } else {
    StatefulPhraseTable::SetParameter(key, value);
  }
}

}
