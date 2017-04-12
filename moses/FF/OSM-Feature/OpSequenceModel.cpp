#include <fstream>
#include "OpSequenceModel.h"
#include "osmHyp.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;
using namespace lm::ngram;

namespace Moses
{

OpSequenceModel::OpSequenceModel(const std::string &line)
  :StatefulFeatureFunction(5, line )
{
  sFactor = 0;
  tFactor = 0;
  numFeatures = 5;
  ReadParameters();
  load_method = util::READ;
}

OpSequenceModel::~OpSequenceModel()
{
  delete OSM;
}

void OpSequenceModel :: readLanguageModel(const char *lmFile)
{
  string unkOp = "_TRANS_SLF_";
  OSM = ConstructOSMLM(m_lmPath.c_str(), load_method);

  State startState = OSM->NullContextState();
  State endState;
  unkOpProb = OSM->Score(startState,unkOp,endState);
}


void OpSequenceModel::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  readLanguageModel(m_lmPath.c_str());
}



void OpSequenceModel:: EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedScores) const
{

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

}


FFState* OpSequenceModel::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  const TargetPhrase &target = cur_hypo.GetCurrTargetPhrase();
  const Bitmap &bitmap = cur_hypo.GetWordsBitmap();
  Bitmap myBitmap(bitmap);
  const Manager &manager = cur_hypo.GetManager();
  const InputType &source = manager.GetSource();
  // const Sentence &sourceSentence = static_cast<const Sentence&>(source);
  osmHypothesis obj;
  vector <string> mySourcePhrase;
  vector <string> myTargetPhrase;
  vector<float> scores;


  //target.GetWord(0)

  //cerr << target <<" --- "<<target.GetSourcePhrase()<< endl;  // English ...

  //cerr << align << endl;   // Alignments ...
  //cerr << cur_hypo.GetCurrSourceWordsRange() << endl;

  //cerr << source <<endl;

// int a = sourceRange.GetStartPos();
// cerr << source.GetWord(a);
  //cerr <<a<<endl;

  //const Sentence &sentence = static_cast<const Sentence&>(curr_hypo.GetManager().GetSource());


  const Range & sourceRange = cur_hypo.GetCurrSourceWordsRange();
  int startIndex  = sourceRange.GetStartPos();
  int endIndex = sourceRange.GetEndPos();
  const AlignmentInfo &align = cur_hypo.GetCurrTargetPhrase().GetAlignTerm();
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
    mySourcePhrase.push_back(source.GetWord(i).GetFactor(sFactor)->GetString().as_string());
    // cerr<<mySourcePhrase[i]<<endl;
  }

  for (size_t i = 0; i < target.GetSize(); i++) {

    if (target.GetWord(i).IsOOV() && sFactor == 0 && tFactor == 0)
      myTargetPhrase.push_back("_TRANS_SLF_");
    else
      myTargetPhrase.push_back(target.GetWord(i).GetFactor(tFactor)->GetString().as_string());

  }


  //cerr<<myBitmap<<endl;

  obj.setState(prev_state);
  obj.constructCepts(alignments,startIndex,endIndex,target.GetSize());
  obj.setPhrases(mySourcePhrase , myTargetPhrase);
  obj.computeOSMFeature(startIndex,myBitmap);
  obj.calculateOSMProb(*OSM);
  obj.populateScores(scores,numFeatures);
  //obj.print();

  /*
    if (bitmap.GetFirstGapPos() == NOT_FOUND)
    {

      int xx;
  	 cerr<<bitmap<<endl;
  	 int a = bitmap.GetFirstGapPos();
  	 obj.print();
      cin>>xx;
    }
    */



  accumulator->PlusEquals(this, scores);

  return obj.saveState();




  //return statePtr;
// return NULL;
}

FFState* OpSequenceModel::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  UTIL_THROW2("Chart decoding not support by OpSequenceModel");

}

const FFState* OpSequenceModel::EmptyHypothesisState(const InputType &input) const
{
  VERBOSE(3,"OpSequenceModel::EmptyHypothesisState()" << endl);

  State startState = OSM->BeginSentenceState();

  return new osmState(startState);
}

std::string OpSequenceModel::GetScoreProducerWeightShortName(unsigned idx) const
{
  return "osm";
}

std::vector<float> OpSequenceModel::GetFutureScores(const Phrase &source, const Phrase &target) const
{
  ParallelPhrase pp(source, target);
  std::map<ParallelPhrase, Scores>::const_iterator iter;
  iter = m_futureCost.find(pp);
//iter = m_coll.find(pp);
  if (iter == m_futureCost.end()) {
    vector<float> scores(numFeatures, 0);
    scores[0] = unkOpProb;
    return scores;
  } else {
    const vector<float> &scores = iter->second;
    return scores;
  }
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

bool OpSequenceModel::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[0];
  return ret;
}

} // namespace
