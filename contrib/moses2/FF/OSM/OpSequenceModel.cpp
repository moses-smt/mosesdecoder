#include <sstream>
#include "OpSequenceModel.h"
#include "../../PhraseBased/Manager.h"
#include "../../PhraseBased/Hypothesis.h"

using namespace std;

namespace Moses2
{
class OSMState: public FFState
{
public:
  int targetLen;

  OSMState()
  {
    // uninitialised
  }

  virtual size_t hash() const
  {
    return (size_t) targetLen;
  }
  virtual bool operator==(const FFState& o) const
  {
    const OSMState& other = static_cast<const OSMState&>(o);
    return targetLen == other.targetLen;
  }

  virtual std::string ToString() const
  {
    stringstream sb;
    sb << targetLen;
    return sb.str();
  }

};

////////////////////////////////////////////////////////////////////////////////////////

OpSequenceModel::OpSequenceModel(size_t startInd, const std::string &line) :
    StatefulFeatureFunction(startInd, line)
{
  ReadParameters();
}

OpSequenceModel::~OpSequenceModel()
{
  // TODO Auto-generated destructor stub
}

FFState* OpSequenceModel::BlankState(MemPool &pool, const System &sys) const
{
  return new (pool.Allocate<OSMState>()) OSMState();
}

void OpSequenceModel::EmptyHypothesisState(FFState &state,
    const ManagerBase &mgr, const InputType &input,
    const Hypothesis &hypo) const
{
  OSMState &stateCast = static_cast<OSMState&>(state);
  stateCast.targetLen = 0;
}

void OpSequenceModel::EvaluateInIsolation(MemPool &pool,
    const System &system, const Phrase<Moses2::Word> &source,
    const TargetPhraseImpl &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void OpSequenceModel::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void OpSequenceModel::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{
  OSMState &stateCast = static_cast<OSMState&>(state);
  stateCast.targetLen = hypo.GetTargetPhrase().GetSize();
}

void OpSequenceModel::EvaluateWhenApplied(const SCFG::Manager &mgr,
    const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
    FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

}
