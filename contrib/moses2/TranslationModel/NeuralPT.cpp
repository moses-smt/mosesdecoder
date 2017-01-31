#include <boost/foreach.hpp>
#include "NeuralPT.h"
#include "../PhraseBased/Hypothesis.h"
#include "../PhraseBased/Sentence.h"
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
, m_maxDevices(1)
, m_factorType(0)
{
  ReadParameters();
  cerr << "NeuralPT::NeuralPT:" << GetNumScores() << endl;
}

void NeuralPT::Load(System &system)
{
  cerr << "NeuralPT::Load start" << endl;
  m_plugin = new amunmt::MosesPlugin();
  m_plugin->initGod(m_modelPath);

  size_t devices = amunmt::MosesPlugin::GetDevices(m_maxDevices);
  std::cerr << devices << std::endl;

  const amunmt::God &god = m_plugin->GetGod();

  CreateVocabMapping(system, god.GetSourceVocab(), m_sourceA2M, m_sourceM2A);
  CreateVocabMapping(system, god.GetTargetVocab(), m_targetA2M, m_targetM2A);

  cerr << "NeuralPT::Load end" << endl;
}

FFState* NeuralPT::BlankState(MemPool &pool, const System &sys) const
{
  cerr << "NeuralPT::BlankState" << endl;
  return new NeuralPTState();
}

void NeuralPT::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
    const InputType &input, const Hypothesis &hypo) const
{
  cerr << "NeuralPT::EmptyHypothesisState start" << endl;
  const Sentence &inputCast = static_cast< const Sentence& >(input);
  std::vector<size_t> amunPhrase = Moses2Amun(inputCast, m_sourceM2A);
  cerr << "amunPhrase=" << amunPhrase.size() << " " << amunPhrase[0] << endl;

  cerr << "NeuralPT::EmptyHypothesisState end" << endl;
}

void NeuralPT::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{
  //cerr << "NeuralPT::EvaluateWhenApplied1" << endl;

}

void NeuralPT::EvaluateWhenApplied(const SCFG::Manager &mgr,
    const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
    FFState &state) const
{
  //cerr << "NeuralPT::EvaluateWhenApplied2" << endl;

}

void NeuralPT::EvaluateBeforeExtending(const Hypotheses &hypos, const Manager &mgr) const
{
  //cerr << "NeuralPT::EvaluateBeforeExtending start" << endl;
  BOOST_FOREACH(const HypothesisBase *hypo, hypos) {
    HypothesisBase *h1 = const_cast<HypothesisBase*>(hypo);
    Hypothesis &h2 = *static_cast<Hypothesis*>(h1);
    EvaluateBeforeExtending(h2, mgr);
  }
  //cerr << "NeuralPT::EvaluateBeforeExtending end" << endl;
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
  if (key == "model") {
    m_modelPath = value;
  }
  else if (key == "devices") {
    m_maxDevices = Scan<size_t>(value);
  }
  else if (key == "factor") {
      m_factorType = Scan<FactorType>(value);
  }
  else {
    StatefulPhraseTable::SetParameter(key, value);
  }
}

void NeuralPT::CreateVocabMapping(
    System &system,
    const amunmt::Vocab &vocab,
    VocabAmun2Moses &a2m,
    VocabMoses2Amun &m2a) const
{
  FactorCollection &fc = system.GetVocab();

  a2m.resize(vocab.size());
  for (size_t i = 0; i < vocab.size(); ++i) {
    const string &str = vocab[i];
    const Factor *factor = fc.AddFactor(str, system, false);
    a2m[i] = factor;
    m2a[factor] = i;
  }
}


std::vector<size_t> NeuralPT::Moses2Amun(const Phrase<Word> &phrase, const VocabMoses2Amun &vocabMapping) const
{
  size_t size = phrase.GetSize();
  std::vector<size_t> ret(size);

  for (size_t i = 0; i < size; ++i) {
    const Word &word = phrase[i];
    const Factor *factor = word[m_factorType];
    VocabMoses2Amun::const_iterator iter = vocabMapping.find(factor);
    if (iter == vocabMapping.end()) {
      // unk
      ret[i] = 1;
    }
    else {
      ret[i] = iter->second;
    }
  }

  return ret;
}

}
