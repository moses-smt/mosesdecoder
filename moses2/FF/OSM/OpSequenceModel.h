#include "../StatefulFeatureFunction.h"
#include "util/mmap.hh"
#include "KenOSM.h"

namespace Moses2
{


class OpSequenceModel : public StatefulFeatureFunction
{
public:
  OSMLM* OSM;
  float unkOpProb;
  int numFeatures;   // Number of features used ...
  int sFactor;  // Source Factor ...
  int tFactor;  // Target Factor ...
  util::LoadMethod load_method; // method to load model

  OpSequenceModel(size_t startInd, const std::string &line);
  virtual ~OpSequenceModel();

  virtual void Load(System &system);

  virtual FFState* BlankState(MemPool &pool, const System &sys) const;
  virtual void EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
                                    const InputType &input, const Hypothesis &hypo) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
                      const TargetPhraseImpl &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const;

  virtual void EvaluateWhenApplied(const ManagerBase &mgr,
                                   const Hypothesis &hypo, const FFState &prevState, Scores &scores,
                                   FFState &state) const;

  virtual void EvaluateWhenApplied(const SCFG::Manager &mgr,
                                   const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
                                   FFState &state) const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  std::string m_lmPath;

  void readLanguageModel(const char *);

};

}


