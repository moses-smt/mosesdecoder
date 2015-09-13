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
  
  PyObject* GetContext() {
    return m_context;
  }
  
  PyObject* GetState() {
    return m_state;
  }
  
  std::string GetLastWord() {
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
  : StatefulFeatureFunction(1, line), m_stateLength(3), m_factor(0),
    m_wrapper(new NMT_Wrapper())
{
  ReadParameters();
  m_wrapper->Init(m_statePath, m_modelPath, m_wrapperPath);
}

void NeuralScoreFeature::ProcessStack(const HypothesisStackNormal& hstack,
                                      const TranslationOptionCollection& to) {
  std::vector<const NeuralScoreState*> states;
  std::vector<std::set<StringPiece> > words;
  
  for (HypothesisStackNormal::const_iterator h = hstack.begin();
       h != hstack.end(); ++h) {
    Hypothesis& hypothesis = **h;
    states.push_back(
      static_cast<const NeuralScoreState*>(hypothesis.GetFFState(GetIndex())));
    
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
            words[i].insert(to.GetTargetPhrase().GetWord(i).GetString(m_factor));
        }
      }
    }
  }
  
  std::cerr << "Collected vocab: " << std::endl;
  for(size_t i = 0; i < words.size(); ++i) {
    std::cerr << i << " " << words[i].size() << std::endl;
  }
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
    
  m_wrapper->GetProb(phrase, context,
                     prevState->GetLastWord(),
                     prevState->GetState(),
                     prob, nextState);
  
  prevState = new NeuralScoreState(context, phrase, nextState);
  nextState = NULL;
  
  newScores[0] = prob;
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
  } else if (key == "model") {
    m_modelPath = value;
  } else if (key == "wrapper-path") {
    m_wrapperPath = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}

