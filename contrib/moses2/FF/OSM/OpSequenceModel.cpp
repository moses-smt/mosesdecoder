#include <sstream>
#include "OpSequenceModel.h"
#include "../../PhraseBased/Manager.h"
#include "../../PhraseBased/Hypothesis.h"
#include "lm/state.hh"

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
  sFactor = 0;
  tFactor = 0;
  numFeatures = 5;
  load_method = util::READ;

  ReadParameters();
}

OpSequenceModel::~OpSequenceModel()
{
  // TODO Auto-generated destructor stub
}

void OpSequenceModel::Load(System &system)
{
  readLanguageModel(m_lmPath.c_str());
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
  /*
  osmHypothesis obj;
  obj.setState(OSM->NullContextState());
  Bitmap myBitmap(source.GetSize());
  vector <string> mySourcePhrase;
  vector <string> myTargetPhrase;
  vector<float> scores;
  vector <int> alignments;
  int startIndex = 0;
  int endIndex = source.GetSize();

  const AlignmentInfo &align = targetPhrase.GetAlignTerm();
  AlignmentInfo::const_iterator iter;

  for (iter = align.begin(); iter != align.end(); ++iter) {
    alignments.push_back(iter->first);
    alignments.push_back(iter->second);
  }

  for (size_t i = 0; i < targetPhrase.GetSize(); i++) {
    if (targetPhrase.GetWord(i).IsOOV() && sFactor == 0 && tFactor == 0)
      myTargetPhrase.push_back("_TRANS_SLF_");
    else
      myTargetPhrase.push_back(targetPhrase.GetWord(i).GetFactor(tFactor)->GetString().as_string());
  }

  for (size_t i = 0; i < source.GetSize(); i++) {
    mySourcePhrase.push_back(source.GetWord(i).GetFactor(sFactor)->GetString().as_string());
  }

  obj.setPhrases(mySourcePhrase , myTargetPhrase);
  obj.constructCepts(alignments,startIndex,endIndex-1,targetPhrase.GetSize());
  obj.computeOSMFeature(startIndex,myBitmap);
  obj.calculateOSMProb(*OSM);
  obj.populateScores(scores,numFeatures);
  estimatedScores.PlusEquals(this, scores);
  */
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

void OpSequenceModel::SetParameter(const std::string& key, const std::string& value)
{

  if (key == "path") {
    m_lmPath = value;
  } else if (key == "support-features") {
    if(value == "no")
      numFeatures = 1;
    else
      numFeatures = 5;
  } else if (key == "input-factor") {
    sFactor = Scan<int>(value);
  } else if (key == "output-factor") {
    tFactor = Scan<int>(value);
  } else if (key == "load") {
    if (value == "lazy") {
      load_method = util::LAZY;
    } else if (value == "populate_or_lazy") {
      load_method = util::POPULATE_OR_LAZY;
    } else if (value == "populate_or_read" || value == "populate") {
      load_method = util::POPULATE_OR_READ;
    } else if (value == "read") {
      load_method = util::READ;
    } else if (value == "parallel_read") {
      load_method = util::PARALLEL_READ;
    } else {
      UTIL_THROW2("Unknown KenLM load method " << value);
    }
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

void OpSequenceModel :: readLanguageModel(const char *lmFile)
{
  string unkOp = "_TRANS_SLF_";
  OSM = ConstructOSMLM(m_lmPath.c_str(), load_method);

  lm::ngram::State startState = OSM->NullContextState();
  lm::ngram::State endState;
  unkOpProb = OSM->Score(startState,unkOp,endState);
}

}
