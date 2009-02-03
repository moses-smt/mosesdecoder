#include "Gibbler.h"

#include "Hypothesis.h"
#include "TranslationOptionCollection.h"
#include "GibblerMaxTransDecoder.h"
#include "StaticData.h"

using namespace std;

namespace Moses {

Sample::Sample(Hypothesis* target_head) : feature_values(target_head->GetScoreBreakdown()){
  
  source_size = target_head->GetWordsBitmap().GetSize();
  
  std::map<int, Hypothesis*> source_order;
  this->target_head = target_head;
  Hypothesis* next = NULL;

  for (Hypothesis* h = target_head; h; h = const_cast<Hypothesis*>(h->GetPrevHypo())) {
    size_t startPos = h->GetCurrSourceWordsRange().GetStartPos();
    SetSourceIndexedHyps(h); 
    if (h->GetPrevHypo()){
      source_order[startPos] = h;  
    }
    else {
      source_order[-1] = h;  
    }
    this->target_tail = h;
    h->m_nextHypo = next;
    next = h;
  }
  
  std::map<int, Hypothesis*>::const_iterator source_it = source_order.begin();
  Hypothesis* prev = NULL;
  this->source_tail = source_it->second;
  
  
  for (; source_it != source_order.end(); source_it++) {
    Hypothesis *h = source_it->second;  
    h->m_sourcePrevHypo = prev;
    if (prev != NULL) 
      prev->m_sourceNextHypo = h;
    this->source_head = h;
    prev = h;
  }
  
  this->source_head->m_sourceNextHypo = NULL;
  this->target_head->m_nextHypo = NULL;
  
  this->source_tail->m_sourcePrevHypo = NULL;
  this->target_tail->m_prevHypo = NULL;
}
 
Sample::~Sample() {
  RemoveAllInColl(cachedSampledHyps);
}  
  
Hypothesis* Sample::GetHypAtSourceIndex(size_t i) {
  std::map<size_t, Hypothesis*>::iterator it = sourceIndexedHyps.find(i);
  assert(it != sourceIndexedHyps.end());
  return it->second;
}

void Sample::SetSourceIndexedHyps(Hypothesis* h) {
  if (!h->m_prevHypo)
    return;
  
  size_t startPos = h->GetCurrSourceWordsRange().GetStartPos();
  size_t endPos = h->GetCurrSourceWordsRange().GetEndPos();
  
  for (size_t i = startPos; i <= endPos; i++) {
    sourceIndexedHyps[i] = h; 
  } 
  
}
  
//x and y are source side positions  
void Sample::FlipNodes(size_t x, size_t y) {
  assert (x != y);
  Hypothesis* hyp1 = GetHypAtSourceIndex(x);
  Hypothesis* hyp2 = GetHypAtSourceIndex(y);
  
  
  WordsRange h1tgtCovered = hyp1->GetCurrTargetWordsRange();
  WordsRange h2tgtCovered = hyp2->GetCurrTargetWordsRange();
  
  if (h1tgtCovered < h2tgtCovered) {
    Hypothesis* hyp1_prevHypo = const_cast<Hypothesis*>(hyp1->m_prevHypo);
    Hypothesis* hyp2_nextHypo = hyp2->m_nextHypo;
    
    hyp2->m_prevHypo = hyp1_prevHypo;
    hyp2->m_nextHypo = hyp1;
    if (hyp1_prevHypo != NULL) {
      hyp1_prevHypo->m_nextHypo = hyp2;
    }
    
    hyp1->m_prevHypo = hyp2;
    hyp1->m_nextHypo = hyp2_nextHypo;
    if (hyp2_nextHypo != NULL) {
      hyp2_nextHypo->m_prevHypo = hyp1;
    }
  }
  else {
    Hypothesis* hyp1_nextHypo = hyp1->m_nextHypo;
    Hypothesis* hyp2_prevHypo = const_cast<Hypothesis*>(hyp2->m_prevHypo);
    
    hyp2->m_prevHypo = hyp1;
    hyp2->m_nextHypo = hyp1_nextHypo;
    if (hyp1_nextHypo != NULL) {
      hyp1_nextHypo->m_prevHypo = hyp2;
    }
    
    hyp1->m_nextHypo = hyp2;
    hyp1->m_prevHypo = hyp2_prevHypo;
    if (hyp2_prevHypo != NULL) {
      hyp2_prevHypo->m_nextHypo = hyp1;
    }
  }
}
  
void Sample::ChangeTarget(const TranslationOption& option, const ScoreComponentCollection& deltaFV)  {
  size_t optionStartPos = option.GetSourceWordsRange().GetStartPos();
  Hypothesis *currHyp = GetHypAtSourceIndex(optionStartPos);
  //cout << "Start pos " << optionStartPos << " hypo " << *currHyp <<  " option: " << option <<  endl;
  const Hypothesis& prevHyp = *currHyp->m_prevHypo;
  Hypothesis *newHyp = new Hypothesis(prevHyp, option);
  cachedSampledHyps.push_back(newHyp);
  
  //cout << "Target head " << target_head << endl;
  CopyTgtSidePtrs(currHyp, newHyp);
  CopySrcSidePtrs(currHyp, newHyp);
  //cout << "Target head " << target_head << endl;

  //Update source side map
  SetSourceIndexedHyps(newHyp); 
  
  //Update target word ranges
  int tgtSizeChange = static_cast<int> (option.GetTargetPhrase().GetSize()) - static_cast<int> (currHyp->GetTargetPhrase().GetSize());
  if (tgtSizeChange != 0) {
    cerr << "Updating tgt word range downstream" << endl;
    UpdateTargetWordRange(newHyp, tgtSizeChange);  
  }
  
  UpdateFeatureValues(deltaFV);
}  

  
  
  
  
void Sample::MergeTarget(const TranslationOption& option, const ScoreComponentCollection& deltaFV)  {
  size_t optionStartPos = option.GetSourceWordsRange().GetStartPos();
  size_t optionEndPos = option.GetSourceWordsRange().GetEndPos();
  
  Hypothesis *currStartHyp = GetHypAtSourceIndex(optionStartPos);
  Hypothesis *currEndHyp = GetHypAtSourceIndex(optionEndPos);
  
  assert(currStartHyp != currEndHyp);
  
  //cout << "Start pos " << optionStartPos << " hypo " << *currHyp <<  " option: " << option <<  endl;
  Hypothesis* prevHyp = NULL;
  Hypothesis* newHyp = NULL;
  
  if (currStartHyp->GetCurrTargetWordsRange() < currEndHyp->GetCurrTargetWordsRange()) {
    prevHyp = const_cast<Hypothesis*> (currStartHyp->m_prevHypo);
    newHyp = new Hypothesis(*prevHyp, option);
    
    //Set the target ptrs
    newHyp->m_prevHypo = prevHyp;
    newHyp->m_nextHypo = currEndHyp->m_nextHypo;
    if (prevHyp) {
      prevHyp->m_nextHypo = newHyp;
    }
    Hypothesis* currHypNext = newHyp->m_nextHypo;
    if (currHypNext) {
      currHypNext->m_prevHypo = newHyp;  
    }
    UpdateHead(currStartHyp, newHyp, target_head);
  } 
  else {
    prevHyp = const_cast<Hypothesis*> (currEndHyp->m_prevHypo);
    newHyp = new Hypothesis(*prevHyp, option);
    
    //Set the target ptrs
    newHyp->m_prevHypo = prevHyp;
    newHyp->m_nextHypo = currStartHyp->m_nextHypo;
    if (prevHyp) {
      prevHyp->m_nextHypo = newHyp;
    }
    Hypothesis* currHypNext = newHyp->m_nextHypo;
    if (currHypNext) {
      currHypNext->m_prevHypo = newHyp;  
    }
    UpdateHead(currEndHyp, newHyp, target_head);
  }
  
  //Set the source ptrs
  newHyp->m_sourcePrevHypo = currStartHyp->m_sourcePrevHypo;
  newHyp->m_sourceNextHypo = currEndHyp->m_sourceNextHypo;
  
  Hypothesis* newHypSourcePrev = newHyp->m_sourcePrevHypo;
  if (newHypSourcePrev) {
    newHypSourcePrev->m_sourceNextHypo = newHyp;
  }
  
  Hypothesis* newHypSourceNext = newHyp->m_sourceNextHypo;
  if (newHypSourceNext) {
    newHypSourceNext->m_sourcePrevHypo = newHyp; 
  }
  
  UpdateHead(currEndHyp, newHyp, source_head);
                    
  //Update source side map
  SetSourceIndexedHyps(newHyp); 
  
  //Update target word ranges
  int newTgtSize = option.GetTargetPhrase().GetSize();
  int prevTgtSize = currStartHyp->GetTargetPhrase().GetSize() + currEndHyp->GetTargetPhrase().GetSize();
  int tgtSizeChange = newTgtSize - prevTgtSize;
  if (tgtSizeChange != 0) {
    cerr << "Updating tgt word range downstream" << endl;
    UpdateTargetWordRange(newHyp, tgtSizeChange);  
  }
    
  UpdateFeatureValues(deltaFV);
  cachedSampledHyps.push_back(newHyp);                  
}
  
  
void Sample::SplitTarget(const TranslationOption& leftTgtOption, const TranslationOption& rightTgtOption,  const ScoreComponentCollection& deltaFV) {
    
  size_t optionStartPos = leftTgtOption.GetSourceWordsRange().GetStartPos();
  Hypothesis *currHyp = GetHypAtSourceIndex(optionStartPos);
  
  //cout << "Start pos " << optionStartPos << " hypo " << *currHyp <<  " option: " << option <<  endl;
  const Hypothesis& prevHyp = *currHyp->m_prevHypo;
  Hypothesis *newLeftHyp = new Hypothesis(prevHyp, leftTgtOption);
  cachedSampledHyps.push_back(newLeftHyp);
  
  Hypothesis *newRightHyp = new Hypothesis(*newLeftHyp, rightTgtOption);
  cachedSampledHyps.push_back(newRightHyp);
  
  //cout << "Target head " << target_head << endl;
  
  //Update tgt ptrs
  newLeftHyp->m_prevHypo = currHyp->m_prevHypo;
  Hypothesis* currHypPrev = const_cast<Hypothesis*>(currHyp->m_prevHypo);
  if (currHypPrev) {
    currHypPrev->m_nextHypo = newLeftHyp;
  }
  newLeftHyp->m_nextHypo = newRightHyp;
  newRightHyp->m_nextHypo = currHyp->m_nextHypo;
  if (currHyp->m_nextHypo) {
    currHyp->m_nextHypo->m_prevHypo = newRightHyp;  
  }
  
  UpdateHead(currHyp, newRightHyp, target_head);
  
  
  //Update src ptrs
  if (newLeftHyp->GetCurrSourceWordsRange() < newRightHyp->GetCurrSourceWordsRange()) { //monotone
    newLeftHyp->m_sourcePrevHypo = currHyp->m_sourcePrevHypo;
    if (currHyp->m_sourcePrevHypo) {
      currHyp->m_sourcePrevHypo->m_sourceNextHypo = newLeftHyp;
    }
    newLeftHyp->m_sourceNextHypo = newRightHyp;
    
    newRightHyp->m_sourcePrevHypo = newLeftHyp;
    newRightHyp->m_sourceNextHypo = currHyp->m_sourceNextHypo;
    if (currHyp->m_sourceNextHypo) {
      currHyp->m_sourceNextHypo->m_sourcePrevHypo = newRightHyp;
    }
    
    UpdateHead(currHyp, newRightHyp, source_head);
    
  }
  else {
    newRightHyp->m_sourcePrevHypo = currHyp->m_sourcePrevHypo;
    if (currHyp->m_sourcePrevHypo) {
      currHyp->m_sourcePrevHypo->m_sourceNextHypo = newRightHyp;
    }
    newRightHyp->m_sourceNextHypo = newLeftHyp;
    
    newLeftHyp->m_sourcePrevHypo = newRightHyp;
    newLeftHyp->m_sourceNextHypo = currHyp->m_sourceNextHypo;
    if (currHyp->m_sourceNextHypo) {
      currHyp->m_sourceNextHypo->m_sourcePrevHypo = newLeftHyp;
    }
    
    UpdateHead(currHyp, newLeftHyp, source_head);
  }
  
  
  //cout << "Target head " << target_head << endl;
  
  //Update source side map
  SetSourceIndexedHyps(newLeftHyp); 
  SetSourceIndexedHyps(newRightHyp); 
  
  //Update target word ranges
  int prevTgtSize = currHyp->GetTargetPhrase().GetSize();
  int newTgtSize = newLeftHyp->GetTargetPhrase().GetSize() + newRightHyp->GetTargetPhrase().GetSize();
  int tgtSizeChange = newTgtSize - prevTgtSize;
  if (tgtSizeChange != 0) {
    cerr << "Updating tgt word range downstream" << endl;
    UpdateTargetWordRange(newRightHyp, tgtSizeChange);  
  }
  
  UpdateFeatureValues(deltaFV);
}  
  
  
  
void Sample::CopyTgtSidePtrs(Hypothesis* currHyp, Hypothesis* newHyp){
  newHyp->m_prevHypo = currHyp->m_prevHypo;
  newHyp->m_nextHypo = currHyp->m_nextHypo;
  Hypothesis* currHypPrev = const_cast<Hypothesis*>(currHyp->m_prevHypo);
  if (currHypPrev) {
    currHypPrev->m_nextHypo = newHyp;
  }
  Hypothesis* currHypNext = currHyp->m_nextHypo;
  if (currHypNext) {
    currHypNext->m_prevHypo = newHyp;  
  }
  
  UpdateHead(currHyp, newHyp, target_head);
}

  
void Sample::CopySrcSidePtrs(Hypothesis* currHyp, Hypothesis* newHyp){
  newHyp->m_sourcePrevHypo = currHyp->m_sourcePrevHypo;
  newHyp->m_sourceNextHypo = currHyp->m_sourceNextHypo;
  Hypothesis* newHypSourcePrev = newHyp->m_sourcePrevHypo;
  if (newHypSourcePrev) {
    newHypSourcePrev->m_sourceNextHypo = newHyp;
  }
  Hypothesis* newHypSourceNext = newHyp->m_sourceNextHypo;
  if (newHypSourceNext) {
    newHypSourceNext->m_sourcePrevHypo = newHyp; 
  }

  UpdateHead(currHyp, newHyp, source_head);
}

void Sample::UpdateHead(Hypothesis* currHyp, Hypothesis* newHyp, Hypothesis *&head) {
  if (head == currHyp)
    head = newHyp;
}
  
void Sample::UpdateTargetWordRange(Hypothesis* hyp, int tgtSizeChange) {
  Hypothesis* nextHyp = const_cast<Hypothesis*>(hyp->GetNextHypo());
  if (!nextHyp)
    return;
    
  for (Hypothesis* h = nextHyp; h; h = const_cast<Hypothesis*>(h->GetNextHypo())){
    WordsRange& range = h->GetCurrTargetWordsRange();
    range.SetStartPos(range.GetStartPos()+tgtSizeChange);
    range.SetEndPos(range.GetEndPos()+tgtSizeChange);
  }
}  
  
void Sample::UpdateFeatureValues(const ScoreComponentCollection& deltaFV) {
  //cout << "Delta: " << deltaFV << endl;
  feature_values.PlusEquals(deltaFV);
  //cout << "New FV " << feature_values << endl;
}  
  
  
void Sampler::Run(Hypothesis* starting, const TranslationOptionCollection* options) {
  size_t iterations = StaticData::Instance().GetNumSamplingIterations();
  vector<GibbsOperator*> operators;
  vector<SampleCollector*> collectors;
  collectors.push_back(new GibblerMaxTransDecoder());
  collectors.push_back(new PrintSampleCollector());
  Sample sample(starting);
  
  operators.push_back(new MergeSplitOperator());
  for (size_t i = 0; i < iterations; ++i) {
    VERBOSE(1,"Gibbs sampling iteration: " << i << endl);
    for (size_t j = 0; j < operators.size(); ++j) {
      VERBOSE(1,"Sampling with operator " << operators[j]->name() << endl);
      operators[j]->doIteration(sample,*options);
      for (size_t k = 0; k < collectors.size(); ++k) {
        collectors[k]->collect(sample);
      }
    }
  }
  
  
  RemoveAllInColl(operators);
  RemoveAllInColl(collectors);

}

void PrintSampleCollector::collect(Sample& sample)  {
      //cout << *(sample.GetSampleHypothesis()) /*->GetTargetPhraseStringRep()*/ << endl;
      cout << "Sampled hypothesis: \"";
      sample.GetSampleHypothesis()->ToStream(cout);
      cout << "\"" << "  " << "Feature values: " << sample.GetFeatureValues() << endl;
    }

}
