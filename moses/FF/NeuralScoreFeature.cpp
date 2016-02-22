#include <vector>
#include <string>
#include <sstream>
#include <deque>
#include <list>
#include <algorithm>

#include <boost/foreach.hpp>

#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/Hypothesis.h"
#include "moses/FF/NeuralScoreFeature.h"
#include "util/string_piece.hh"

namespace Moses
{

class NeuralScoreState : public FFState
{
public:
  NeuralScoreState(StateInfoPtr state, const std::deque<std::string>& context,
                   const std::vector<std::string>& lastWords)
  : m_state(state),
    m_lastWord(lastWords.back()),
    m_lastContext(context) {
    for(size_t i = 0; i  < lastWords.size(); ++i)
      m_lastContext.push_back(lastWords[i]);
  }

  NeuralScoreState(StateInfoPtr state)
  : m_state(state),
    m_lastWord("") {}

  std::string ToString() const {
    std::stringstream ss;
    for(size_t i = 0; i < m_lastContext.size(); ++i) {
      if(i != 0)
        ss << " ";
      ss << m_lastContext[i];
    }
    return ss.str();
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

  const std::string& GetLastWord() const {
    return m_lastWord;
  }
  
  const std::deque<std::string>& GetContext() const {
    return m_lastContext;
  }
  
  StateInfoPtr GetState() const {
    return m_state;
  }

private:
  StateInfoPtr m_state;
  std::string m_lastWord;
  std::deque<std::string> m_lastContext;
};

void NeuralScoreFeature::InitializeForInput(ttasksptr const& ttask) {  
  if(!m_nmt.get())  {
    boost::mutex::scoped_lock lock(m_mutex);
    size_t device = m_threadId++ % m_models.size();
    m_nmt.reset(new NMT(m_models[device], m_sourceVocab, m_targetVocab));
    m_targetWords.reset(new std::set<std::string>());
    m_nmt->SetDevice();  
  }
  else {
    m_nmt.reset(new NMT(m_nmt->GetModel(), m_sourceVocab, m_targetVocab));
  }
  m_nmt->ClearStates();
  m_targetWords->clear();
}

void NeuralScoreFeature::CleanUpAfterSentenceProcessing(ttasksptr const& ttask) {
}

const FFState* NeuralScoreFeature::EmptyHypothesisState(const InputType &input) const {
  UTIL_THROW_IF2(input.GetType() != SentenceInput,
                 "This feature function requires the Sentence input type");
  const Sentence& sentence = static_cast<const Sentence&>(input);
  
  if(m_filteredSoftmax) {
    m_targetWords->insert("</s>");
    m_targetWords->insert("UNK");
    m_nmt->FilterTargetVocab(*m_targetWords);
  }
  
  std::vector<std::string> sourceSentence;
  for(size_t i = 0; i < sentence.GetSize(); i++)
    sourceSentence.push_back(sentence.GetWord(i).GetString(0).as_string());
  
  m_nmt->CalcSourceContext(sourceSentence);
  
  return new NeuralScoreState(m_nmt->EmptyState());
}

NeuralScoreFeature::NeuralScoreFeature(const std::string &line)
  : StatefulFeatureFunction(2, line), m_batchSize(1000), m_stateLength(5),
    m_factor(0), m_maxDevices(1), m_filteredSoftmax(false),
    m_mode("precalculate"), m_threadId(0)
{
  ReadParameters();
  
  size_t devices = NMT::GetDevices(m_maxDevices);
  std::cerr << devices << std::endl;
  for(size_t device = 0; device < devices; ++device)
    m_models.push_back(NMT::NewModel(m_modelPath, device));
  
  m_sourceVocab = NMT::NewVocab(m_sourceVocabPath);
  m_targetVocab = NMT::NewVocab(m_targetVocabPath);
}

void NeuralScoreFeature::RescoreStack(std::vector<Hypothesis*>& hyps, size_t index) {
  if(m_mode != "rescore")
    return;
  
  std::map<int, const NeuralScoreState*> states;
  
  if(!m_pbl.get()) {
    m_pbl.reset(new PrefsByLength());
  }
  m_pbl->clear();
  
  BOOST_FOREACH(Hypothesis* nextHyp, hyps) {
    if(nextHyp->GetId()) {
      const Hypothesis* prevHyp = nextHyp->GetPrevHypo();
      
      const FFState* prevffstate = prevHyp->GetFFState(index);
      const NeuralScoreState* prevState
        = static_cast<const NeuralScoreState*>(prevffstate);
      
      size_t hypId = prevHyp->GetId();
      states[hypId] = prevState;
    
      const TargetPhrase& tp = nextHyp->GetCurrTargetPhrase();
  
      Prefix prefix;
      for(size_t i = 0; i < tp.GetSize(); ++i) {
        prefix.push_back(tp.GetWord(i).GetString(m_factor).as_string());
        if(m_pbl->size() < prefix.size())
          m_pbl->resize(prefix.size());
  
        (*m_pbl)[prefix.size() - 1][prefix][hypId] = Payload();
      }
      
      if(nextHyp->IsSourceCompleted()) {
        prefix.push_back("</s>");
        if(m_pbl->size() < prefix.size())
          m_pbl->resize(prefix.size());
  
        (*m_pbl)[prefix.size() - 1][prefix][hypId] = Payload();
      }
    }
    else {
      return;
    }
  }
  
  for(size_t l = 0; l < m_pbl->size(); l++) {
    Prefixes& prefixes = (*m_pbl)[l];
  
    std::vector<std::string> allWords;
    std::vector<std::string> allLastWords;
    std::vector<StateInfoPtr> allStates;
  
    for(Prefixes::iterator it = prefixes.begin(); it != prefixes.end(); it++) {
      const Prefix& prefix = it->first;
      BOOST_FOREACH(SP& hyp, it->second) {
        size_t hypId = hyp.first;
        allWords.push_back(prefix[l]);
        StateInfoPtr state;
        if(prefix.size() == 1) {
          state = states[hypId]->GetState();
          allLastWords.push_back(states[hypId]->GetLastWord());
        }
        else {
          Prefix prevPrefix = prefix;
          prevPrefix.pop_back();
          state = (*m_pbl)[prevPrefix.size() - 1][prevPrefix][hypId].state_;
          allLastWords.push_back(prevPrefix.back());
        }
        allStates.push_back(state);
      }
    }
  
    std::vector<double> allProbs;
    std::vector<StateInfoPtr> allOutStates;
    std::vector<bool> unks;
    
    BatchProcess(allWords,
                 allLastWords,
                 allStates,
                 /** out **/
                 allProbs,
                 allOutStates,
                 unks);
    
    size_t k = 0;
    for(Prefixes::iterator it = prefixes.begin(); it != prefixes.end(); it++) {
      BOOST_FOREACH(SP& hyp, it->second) {
        Payload& payload = hyp.second;
        payload.logProb_ = allProbs[k];
        payload.state_ = allOutStates[k];
        payload.known_ = unks[k];
        k++;
      }
    }
  }
  
  BOOST_FOREACH(Hypothesis* nextHyp, hyps) {
    int prevId = nextHyp->GetPrevHypo()->GetId();
    const TargetPhrase& tp = nextHyp->GetCurrTargetPhrase();
    
    float prob = 0;
    size_t unks = 0;
    StateInfoPtr newStateInfo;
    
    Prefix phrase;
    for(size_t i = 0; i < tp.GetSize(); ++i) {
      std::string word = tp.GetWord(i).GetString(m_factor).as_string();
      phrase.push_back(word);
    }
    if(nextHyp->IsSourceCompleted()) {
      phrase.push_back("</s>");
    }
    
    Prefix prefix;
    for(size_t i = 0; i < phrase.size(); i++) {
      prefix.push_back(phrase[i]);
      if(!const_cast<PrefsByLength&>(*m_pbl)[prefix.size() - 1][prefix].count(prevId)) {
        BOOST_FOREACH(std::string s, prefix) {
          std::cerr << s << " ";
        }
        std::cerr << std::endl;
      }
    
      Payload& payload = const_cast<PrefsByLength&>(*m_pbl)[prefix.size() - 1][prefix][prevId];
      newStateInfo = payload.state_;
      prob += payload.logProb_;
      unks += payload.known_ ? 0 : 1;
    }
    
    NeuralScoreState* newState =
      new NeuralScoreState(newStateInfo,
                           states[prevId]->GetContext(),
                           phrase);
    
    std::vector<float> newScores(2, 0.0);
    newScores[0] = prob;
    newScores[1] = unks;

    newState->LimitLength(m_stateLength);
    
    ScoreComponentCollection& accumulator = nextHyp->GetCurrScoreBreakdown();
    accumulator.PlusEquals(this, newScores);
    nextHyp->Recalc();
    nextHyp->SetFFState(index, newState);
  }
}

void NeuralScoreFeature::ProcessStack(Collector& collector, size_t index) {
  if(m_mode != "precalculate")
    return;
  
  std::map<int, const NeuralScoreState*> states;
  
  bool first = true;
  size_t covered;
  size_t total;
  
  if(!m_pbl.get()) {
    m_pbl.reset(new PrefsByLength());
  }
  m_pbl->clear();
  
  BOOST_FOREACH(const Hypothesis* h, collector.GetHypotheses()) {
    const Hypothesis& hypothesis = *h;
  
    const FFState* ffstate = hypothesis.GetFFState(index);
    const NeuralScoreState* state
      = static_cast<const NeuralScoreState*>(ffstate);
  
    if(first) {
      const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
      covered = hypoBitmap.GetNumWordsCovered();
      total = hypoBitmap.GetSize();
      first = false;
    }
  
    size_t hypId = hypothesis.GetId();
    states[hypId] = state;
  
    BOOST_FOREACH(const TranslationOptionList* tol, collector.GetOptions(hypId)) {
      TranslationOptionList::const_iterator iter;
      for (iter = tol->begin() ; iter != tol->end() ; ++iter) {
        const TranslationOption& to = **iter;
        const TargetPhrase& tp = to.GetTargetPhrase();
  
        Prefix prefix;
        for(size_t i = 0; i < tp.GetSize(); ++i) {
          prefix.push_back(tp.GetWord(i).GetString(m_factor).as_string());
          if(m_pbl->size() < prefix.size())
            m_pbl->resize(prefix.size());
  
          (*m_pbl)[prefix.size() - 1][prefix][hypId] = Payload();
        }
        if(total - covered == to.GetSize()) {
          prefix.push_back("</s>");
          if(m_pbl->size() < prefix.size())
            m_pbl->resize(prefix.size());
  
          (*m_pbl)[prefix.size() - 1][prefix][hypId] = Payload();
        }
      }
    }
  }
  
  for(size_t l = 0; l < m_pbl->size(); l++) {
    Prefixes& prefixes = (*m_pbl)[l];
  
    std::vector<std::string> allWords;
    std::vector<std::string> allLastWords;
    std::vector<StateInfoPtr> allStates;
  
    for(Prefixes::iterator it = prefixes.begin(); it != prefixes.end(); it++) {
      const Prefix& prefix = it->first;
      BOOST_FOREACH(SP& hyp, it->second) {
        size_t hypId = hyp.first;
        allWords.push_back(prefix[l]);
        StateInfoPtr state;
        if(prefix.size() == 1) {
          state = states[hypId]->GetState();
          allLastWords.push_back(states[hypId]->GetLastWord());
        }
        else {
          Prefix prevPrefix = prefix;
          prevPrefix.pop_back();
          state = (*m_pbl)[prevPrefix.size() - 1][prevPrefix][hypId].state_;
          allLastWords.push_back(prevPrefix.back());
        }
        allStates.push_back(state);
      }
    }
  
    std::vector<double> allProbs;
    std::vector<StateInfoPtr> allOutStates;
    std::vector<bool> unks;
    
    BatchProcess(allWords,
                 allLastWords,
                 allStates,
                 /** out **/
                 allProbs,
                 allOutStates,
                 unks);
    
    size_t k = 0;
    for(Prefixes::iterator it = prefixes.begin(); it != prefixes.end(); it++) {
      BOOST_FOREACH(SP& hyp, it->second) {
        Payload& payload = hyp.second;
        payload.logProb_ = allProbs[k];
        payload.state_ = allOutStates[k];
        payload.known_ = unks[k];
        k++;
      }
    }
  }
}

void NeuralScoreFeature::BatchProcess(
  const std::vector<std::string>& nextWords,
  const std::vector<std::string>& lastWords,
  std::vector<StateInfoPtr>& inputStates,
  std::vector<double>& logProbs,
  std::vector<StateInfoPtr>& outputStates,
  std::vector<bool>& unks) {
  
    size_t items = nextWords.size();
    size_t batches = ceil(items/(float)m_batchSize);
    for(size_t i = 0; i < batches; ++i) {
      size_t thisBatchStart = i * m_batchSize;
      size_t thisBatchEnd = std::min(thisBatchStart + m_batchSize, items);
      
      
      std::vector<std::string> nextWordsBatch(nextWords.begin() + thisBatchStart,
                                              nextWords.begin() + thisBatchEnd);
      std::vector<std::string> lastWordsBatch(lastWords.begin() + thisBatchStart,
                                              lastWords.begin() + thisBatchEnd);
      std::vector<StateInfoPtr> inputStatesBatch(inputStates.begin() + thisBatchStart,
                                              inputStates.begin() + thisBatchEnd);

      std::vector<double> logProbsBatch;
      std::vector<StateInfoPtr> nextStatesBatch;
      std::vector<bool> unksBatch;
      
      m_nmt->MakeStep(nextWordsBatch,
                lastWordsBatch,
                inputStatesBatch,
                /** out **/
                logProbsBatch,
                nextStatesBatch,
                unksBatch);
          
      logProbs.insert(logProbs.end(), logProbsBatch.begin(), logProbsBatch.end());
      outputStates.insert(outputStates.end(), nextStatesBatch.begin(), nextStatesBatch.end());
      unks.insert(unks.end(), unksBatch.begin(), unksBatch.end());
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
{ }

void NeuralScoreFeature::EvaluateTranslationOptionListWithSourceContext(const InputType &input
    , const TranslationOptionList &translationOptionList) const
{
  if(m_filteredSoftmax) {
    TranslationOptionList::const_iterator iter;
    for (iter = translationOptionList.begin();
         iter != translationOptionList.end(); ++iter) {
      const TranslationOption& to = **iter;
      const TargetPhrase& tp = to.GetTargetPhrase();
    
      for(size_t i = 0; i < tp.GetSize(); ++i) {
        std::string temp = tp.GetWord(i).GetString(m_factor).as_string();
        const_cast<std::set<std::string>&>(*m_targetWords).insert(temp);
      }
    }
  }
}

FFState* NeuralScoreFeature::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  std::vector<float> newScores(m_numScoreComponents, 0);
  
  const TargetPhrase& tp = cur_hypo.GetCurrTargetPhrase();
  Prefix phrase;
  
  for(size_t i = 0; i < tp.GetSize(); ++i) {
    std::string word = tp.GetWord(i).GetString(m_factor).as_string();
    phrase.push_back(word);
  }
  if(cur_hypo.IsSourceCompleted()) {
    phrase.push_back("</s>");
  }
  
  float prob = 0;
  size_t unks = 0;
  const StateInfoPtr prevStateInfo = static_cast<const NeuralScoreState*>(prev_state)->GetState();
  StateInfoPtr newStateInfo;

  if(m_mode == "precalculate") {
    int prevId = cur_hypo.GetPrevHypo()->GetId();
    Prefix prefix;
    for(size_t i = 0; i < phrase.size(); i++) {
      prefix.push_back(phrase[i]);
      if(!const_cast<PrefsByLength&>(*m_pbl)[prefix.size() - 1][prefix].count(prevId)) {
        BOOST_FOREACH(std::string s, prefix) {
          std::cerr << s << " ";
        }
        std::cerr << std::endl;
      }
  
      Payload& payload = const_cast<PrefsByLength&>(*m_pbl)[prefix.size() - 1][prefix][prevId];
      newStateInfo = payload.state_;
      prob += payload.logProb_;
      unks += payload.known_ ? 0 : 1;
    }
  }
  else if(m_mode == "naive") {
    bool isFirstWord = (cur_hypo.GetPrevHypo()->GetWordsBitmap().GetNumWordsCovered() == 0);
    std::string lastWord = static_cast<const NeuralScoreState*>(prev_state)->GetLastWord();
    m_nmt->OnePhrase(phrase, lastWord, isFirstWord, prevStateInfo, prob, unks, newStateInfo);
  }
  else if(m_mode == "rescore") {
    //
  }
  
  NeuralScoreState* newState =
    new NeuralScoreState(newStateInfo,
                         static_cast<const NeuralScoreState*>(prev_state)->GetContext(),
                         phrase);

  newScores[0] = prob;
  newScores[1] = unks;

  accumulator->PlusEquals(this, newScores);

  newState->LimitLength(m_stateLength);
  return newState;
}

FFState* NeuralScoreFeature::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new NeuralScoreState(StateInfoPtr());
}

void NeuralScoreFeature::SetParameter(const std::string& key, const std::string& value)
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
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}

