#include <sstream>
#include "OpSequenceModel.h"
#include "osmHyp.h"
#include "lm/state.hh"
#include "../../PhraseBased/Manager.h"
#include "../../PhraseBased/Hypothesis.h"
#include "../../PhraseBased/TargetPhraseImpl.h"
#include "../../PhraseBased/Sentence.h"
#include "../../TranslationModel/UnknownWordPenalty.h"
#include "../../System.h"

using namespace std;

namespace Moses2
{

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
  return new (pool.Allocate<osmState>()) osmState();
}

void OpSequenceModel::EmptyHypothesisState(FFState &state,
    const ManagerBase &mgr, const InputType &input,
    const Hypothesis &hypo) const
{
  lm::ngram::State startState = OSM->BeginSentenceState();

  osmState &stateCast = static_cast<osmState&>(state);
  stateCast.setState(startState);
}

void OpSequenceModel::EvaluateInIsolation(MemPool &pool,
    const System &system, const Phrase<Moses2::Word> &source,
    const TargetPhraseImpl &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
  osmHypothesis obj;
  obj.setState(OSM->NullContextState());

  Bitmap myBitmap (pool, source.GetSize());
  myBitmap.Init(std::vector<bool>());

  vector <string> mySourcePhrase;
  vector <string> myTargetPhrase;
  vector<float> scoresVec;
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
    if (&targetPhrase.pt == system.featureFunctions.GetUnknownWordPenalty() && sFactor == 0 && tFactor == 0)
      myTargetPhrase.push_back("_TRANS_SLF_");
    else
      myTargetPhrase.push_back(targetPhrase[i][tFactor]->GetString().as_string());
  }

  for (size_t i = 0; i < source.GetSize(); i++) {
    mySourcePhrase.push_back(source[i][sFactor]->GetString().as_string());
  }

  obj.setPhrases(mySourcePhrase , myTargetPhrase);
  obj.constructCepts(alignments,startIndex,endIndex-1,targetPhrase.GetSize());
  obj.computeOSMFeature(startIndex,myBitmap);
  obj.calculateOSMProb(*OSM);
  obj.populateScores(scoresVec,numFeatures);

  SCORE weightedScore = Scores::CalcWeightedScore(system, *this,
                        scoresVec.data());
  estimatedScore += weightedScore;

}

void OpSequenceModel::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
  UTIL_THROW2("Not implemented");
}

void OpSequenceModel::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{
  const TargetPhrase<Moses2::Word> &target = hypo.GetTargetPhrase();
  const Bitmap &bitmap = hypo.GetBitmap();
  Bitmap myBitmap(bitmap);
  const ManagerBase &manager = hypo.GetManager();
  const InputType &source = manager.GetInput();
  const Sentence &sourceSentence = static_cast<const Sentence&>(source);

  osmHypothesis obj;
  vector <string> mySourcePhrase;
  vector <string> myTargetPhrase;
  vector<float> scoresVec;


  //target.GetWord(0)

  //cerr << target <<" --- "<<target.GetSourcePhrase()<< endl;  // English ...

  //cerr << align << endl;   // Alignments ...
  //cerr << cur_hypo.GetCurrSourceWordsRange() << endl;

  //cerr << source <<endl;

// int a = sourceRange.GetStartPos();
// cerr << source.GetWord(a);
  //cerr <<a<<endl;

  //const Sentence &sentence = static_cast<const Sentence&>(curr_hypo.GetManager().GetSource());


  const Range & sourceRange = hypo.GetInputPath().range;
  int startIndex  = sourceRange.GetStartPos();
  int endIndex = sourceRange.GetEndPos();
  const AlignmentInfo &align = hypo.GetTargetPhrase().GetAlignTerm();
  // osmState * statePtr;

  vector <int> alignments;



  AlignmentInfo::const_iterator iter;

  for (iter = align.begin(); iter != align.end(); ++iter) {
    //cerr << iter->first << "----" << iter->second << " ";
    alignments.push_back(iter->first);
    alignments.push_back(iter->second);
  }


  //cerr<<bitmap<<endl;
  //cerr<<startIndex<<" "<<endIndex<<endl;


  for (int i = startIndex; i <= endIndex; i++) {
    myBitmap.SetValue(i,0); // resetting coverage of this phrase ...
    mySourcePhrase.push_back(sourceSentence[i][sFactor]->GetString().as_string());
    // cerr<<mySourcePhrase[i]<<endl;
  }

  for (size_t i = 0; i < target.GetSize(); i++) {
    if (&target.pt == mgr.system.featureFunctions.GetUnknownWordPenalty() && sFactor == 0 && tFactor == 0)
      myTargetPhrase.push_back("_TRANS_SLF_");
    else
      myTargetPhrase.push_back(target[i][tFactor]->GetString().as_string());

  }


  //cerr<<myBitmap<<endl;

  obj.setState(&prevState);
  obj.constructCepts(alignments,startIndex,endIndex,target.GetSize());
  obj.setPhrases(mySourcePhrase , myTargetPhrase);
  obj.computeOSMFeature(startIndex,myBitmap);
  obj.calculateOSMProb(*OSM);
  obj.populateScores(scoresVec,numFeatures);
  //obj.print();

  scores.PlusEquals(mgr.system, *this, scoresVec);

  osmState &stateCast = static_cast<osmState&>(state);
  obj.saveState(stateCast);
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
