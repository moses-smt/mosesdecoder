#include <vector>
#include <string>
#include <sstream>
#include <deque>
#include <list>
#include <set>
#include <algorithm>
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/Hypothesis.h"
#include "moses/FF/NMT/NMT_Wrapper.h"
#include "moses/FF/NeuralScoreFeature.h"
#include "util/string_piece.hh"

namespace Moses
{
 
class NeuralScoreState : public FFState
{  
public:
  NeuralScoreState(PyObject* context, const std::string& lastWord, PyObject* state)
  : m_context(context), m_lastWord(lastWord), m_state(state) {
    m_lastContext.push_back(m_lastWord);
  }

  NeuralScoreState(PyObject* context, const std::vector<std::string>& lastPhrase, PyObject* state)
  : m_context(context), m_lastWord(lastPhrase.back()), m_state(state) {
    for(size_t i = 0; i < lastPhrase.size(); i++)
      m_lastContext.push_back(lastPhrase[i]);
  }
    
  int Compare(const FFState& other) const
  {
    const NeuralScoreState &otherState = static_cast<const NeuralScoreState&>(other);
    if(m_lastContext.size() == otherState.m_lastContext.size() &&
        std::equal(m_lastContext.begin(),
                   m_lastContext.end(),
                   otherState.m_lastContext.begin()))
      return 0;
    return (std::lexicographical_compare(m_lastContext.begin(), m_lastContext.end(),
                   otherState.m_lastContext.begin(),
                   otherState.m_lastContext.end())) ? -1 : +1;
  }

  void LimitLength(size_t length) {
    while(m_lastContext.size() > length)
      m_lastContext.pop_front();
  }
  
  PyObject* GetContext() const {
    return m_context;
  }
  
  PyObject* GetState() const {
    return m_state;
  }
  
  std::string GetLastWord() const {
    return m_lastWord;
  }
  
private:
  PyObject* m_context;
  std::string m_lastWord;
  std::deque<std::string> m_lastContext;
  PyObject* m_state;
};

const FFState* NeuralScoreFeature::EmptyHypothesisState(const InputType &input) const {
  UTIL_THROW_IF2(input.GetType() != SentenceInput,
                 "This feature function requires the Sentence input type");
  
  const Sentence& sentence = static_cast<const Sentence&>(input);
  std::stringstream ss;
  for(size_t i = 0; i < sentence.GetSize(); i++)
    ss << sentence.GetWord(i).GetString(0) << " ";
  
  std::string sentenceString = Trim(ss.str());
  PyObject* pyContextVectors;
  m_wrapper->GetContextVectors(sentenceString, pyContextVectors);
  
  return new NeuralScoreState(pyContextVectors, "", NULL);
}

NeuralScoreFeature::NeuralScoreFeature(const std::string &line)
  : StatefulFeatureFunction(1, line), m_preCalc(0), m_stateLength(3), m_factor(0),
    m_wrapper(new NMT_Wrapper())
{
  ReadParameters();
  m_wrapper->Init(m_statePath, m_modelPath, m_wrapperPath);
}

void NeuralScoreFeature::ProcessStack(const HypothesisStackNormal& hstack,
                                      const TranslationOptionCollection& to,
                                      size_t index) {
  if(!m_preCalc)
    return;
  
  PyObject* sourceContext = 0;
  std::vector<PyObject*> states;
  std::vector<std::string> lastWords;

  currWords_.clear();
  logProbs_.clear();
  outputStates_.clear();
  prevHypIds_.clear();
  
  std::vector<std::set<std::string> > words;
  std::size_t hypPos = 0;
  for (HypothesisStackNormal::const_iterator h = hstack.begin();
       h != hstack.end(); ++h) {
    Hypothesis& hypothesis = **h;
    prevHypIds_[hypothesis.GetId()] = hypPos;
    hypPos++;
    
    const FFState* ffstate = hypothesis.GetFFState(index);
    const NeuralScoreState* state
      = static_cast<const NeuralScoreState*>(ffstate);
    
    states.push_back(state->GetState());
    //std::cerr << "STATES2 : " << states.back() << std::endl;
    lastWords.push_back(state->GetLastWord());
    
    if(sourceContext == 0)
      sourceContext = state->GetContext();
    
    const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
    const size_t hypoFirstGapPos = hypoBitmap.GetFirstGapPos();
    const size_t sourceSize = hypothesis.GetInput().GetSize();  
    for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos) {
      TranslationOptionList const* tol;
      size_t endPos = startPos;
      for (tol = to.GetTranslationOptionList(startPos, endPos);
           tol && endPos < sourceSize;
           tol = to.GetTranslationOptionList(startPos, ++endPos)) {
       
        if (tol->size() == 0 || hypoBitmap.Overlap(WordsRange(startPos, endPos)))
          continue;
        
        TranslationOptionList::const_iterator iter;
        for (iter = tol->begin() ; iter != tol->end() ; ++iter) {
          const TranslationOption& to = **iter;
          const TargetPhrase& tp = to.GetTargetPhrase();
          if(tp.GetSize() > words.size())
            words.resize(tp.GetSize());
          for(size_t i = 0; i < tp.GetSize(); ++i)
            words[i].insert(to.GetTargetPhrase().GetWord(i).GetString(m_factor).as_string());
        }
      }
    }
  }
  
  //void GetNextLogProbStates(
  //        const std::vector<std::string>& nextWords,
  //        PyObject* pyContextVectors,
  //        const std::vector< std::string >& lastWords,
  //        std::vector<PyObject*>& inputStates,
  //        std::vector<double>& logProbs,
  //        std::vector<PyObject*>& nextStates);

  
  // construct TRIE that has N states in nodes
  
  // only first word for now
  if(!words.empty()) {  
    currWords_.insert(currWords_.end(), words[0].begin(), words[0].end());
    std::cerr << "Collected vocab test: " << currWords_.size() << " " << states.size() << std::endl;
  
    std::vector<std::string> allWords;
    std::vector<PyObject*> allStates;
    
    std::vector<std::string> allLastWords;
    for(size_t i = 0; i < currWords_.size(); ++i) {
      allStates.insert(allStates.end(), states.begin(), states.end());
      allLastWords.insert(allLastWords.end(), lastWords.begin(), lastWords.end());
      allWords.insert(allWords.end(), states.size(), currWords_[i]);
    }
    
    size_t M = currWords_.size();
    size_t N = states.size();
    logProbs_.resize(N, std::vector<double>(M));
    outputStates_.resize(N, std::vector<PyObject*>(M));
    
    std::vector<double> allProbs;
    std::vector<PyObject*> allOutStates;
    
    m_wrapper->GetNextLogProbStates(allWords,
                                    sourceContext,
                                    allLastWords,
                                    allStates,
                                    allProbs,
                                    allOutStates);
    
    for(size_t k = 0; k < allProbs.size(); ++k) {
      size_t i = k / currWords_.size();
      size_t j = k % currWords_.size();
      
      logProbs_[i][j] = allProbs[k];
      if(allOutStates[k] == 0)
        std::cerr << "Caught zero!" << std::endl;
      outputStates_[i][j] = allOutStates[k];
    }
    
    //m_wrapper->GetProb(currWords_, sourceContext, lastWords, states,
    //                   logProbs_, outputStates_);
  }
  std::cerr << "done" << std::endl;
}

void NeuralScoreFeature::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{}

void NeuralScoreFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedFutureScore) const
{}

void NeuralScoreFeature::EvaluateTranslationOptionListWithSourceContext(const InputType &input
    , const TranslationOptionList &translationOptionList) const
{}

FFState* NeuralScoreFeature::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  NeuralScoreState* prevState = static_cast<NeuralScoreState*>(
                                  const_cast<FFState*>(prev_state));
  
  PyObject* context = prevState->GetContext();
  std::vector<float> newScores(m_numScoreComponents);
  
  double prob = 0;
  PyObject* nextState = NULL;
  const TargetPhrase& tp = cur_hypo.GetCurrTargetPhrase();
  std::vector<std::string> phrase;
  for(size_t i = 0; i < tp.GetSize(); ++i) {
    std::string word = tp.GetWord(i).GetString(m_factor).as_string();
    phrase.push_back(word);
  }
  
  int prevId = cur_hypo.GetPrevHypo()->GetId();
  size_t prevIndex = const_cast<std::map<int, size_t>&>(prevHypIds_)[prevId];
  
  if(!m_preCalc) {
    m_wrapper->GetProb(phrase, context,
                       prevState->GetLastWord(),
                       prevState->GetState(),
                       prob, nextState);
    prevState = new NeuralScoreState(context, phrase, nextState);
    nextState = NULL;
    
   newScores[0] = prob;
  }
  else {
    std::vector<std::string>::const_iterator it
      = std::lower_bound(currWords_.begin(), currWords_.end(), phrase[0]);
    size_t pos = std::distance(currWords_.begin(), it);
    if(it == currWords_.end()) {
      std::cerr << "Error: could not find " << phrase[0] << std::endl; 
    }
    else {
      std::cerr << "Found " << phrase[0] << " at " << pos << std::endl;
    }
    std::cerr << "Hypothesis " << prevId << " was in stack at " << prevIndex << std::endl;
    
    
    PyObject* pyState = const_cast<std::vector<std::vector<PyObject*> >&>(outputStates_)[prevIndex][pos];
    prevState = new NeuralScoreState(context, phrase, pyState);
    nextState = NULL;
  
    newScores[0] = const_cast<std::vector<std::vector<double> >&>(logProbs_)[prevIndex][pos];
    std::cerr << "Got logProb " << newScores[0] << std::endl;
  }
  accumulator->PlusEquals(this, newScores);
  
  prevState->LimitLength(m_stateLength);
  return prevState;
}

FFState* NeuralScoreFeature::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new NeuralScoreState(NULL, "", NULL);
}

void NeuralScoreFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "state") {
    m_statePath = value;
  } else if (key == "state-length") {
    m_stateLength = Scan<size_t>(value);
  } else if (key == "precalculate") {
    m_preCalc = Scan<bool>(value);
  } else if (key == "model") {
    m_modelPath = value;
  } else if (key == "wrapper-path") {
    m_wrapperPath = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}

