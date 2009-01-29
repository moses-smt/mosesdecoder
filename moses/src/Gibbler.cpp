#include "Gibbler.h"

#include "Hypothesis.h"
#include "TranslationOptionCollection.h"
#include "GibblerMaxTransDecoder.h"

using namespace std;

namespace Moses {

Sample::Sample(Hypothesis* target_head) {
  
  source_size = target_head->GetWordsBitmap().GetSize();
  
  std::map<size_t, Hypothesis*> source_order;
  this->target_head = target_head;
  Hypothesis* next = NULL;

  for (Hypothesis* h = target_head; h->GetId() > 0; h = const_cast<Hypothesis*>(h->GetPrevHypo())) {
    size_t startPos = h->GetCurrSourceWordsRange().GetStartPos();
    SetSourceIndexedHyps(h); 
    source_order[startPos] = h;
    this->target_tail = h;
    h->m_nextHypo = next;
    next = h;
  }
  
  std::map<size_t, Hypothesis*>::const_iterator source_it = source_order.begin();
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
}
 
Hypothesis* Sample::GetHypAtSourceIndex(size_t i) {
  std::map<size_t, Hypothesis*>::iterator it = sourceIndexedHyps.find(i);
  assert(it != sourceIndexedHyps.end());
  return it->second;
}

void Sample::SetSourceIndexedHyps(Hypothesis* h) {
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
  
  const Hypothesis& prevHyp = *currHyp->m_prevHypo;
  Hypothesis newHyp(prevHyp, option);
  
  CopyTgtSidePtrs(currHyp, &newHyp);
  CopySrcSidePtrs(currHyp, &newHyp);

  //Update source side map
  SetSourceIndexedHyps(&newHyp); 
  
  UpdateFeatureValues(deltaFV);
}  

void Sample::UpdateFeatureValues(const ScoreComponentCollection& deltaFV) {
  feature_values.PlusEquals(deltaFV);
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
}

  
void Sampler::Run(Hypothesis* starting, const TranslationOptionCollection* options) {
  size_t iterations = 5;
  vector<GibbsOperator*> operators;
  vector<SampleCollector*> collectors;
  collectors.push_back(new GibblerMaxTransDecoder());
  collectors.push_back(new PrintSampleCollector());
  Sample sample(starting);
  
  operators.push_back(new MergeSplitOperator());
  for (size_t i = 0; i < iterations; ++i) {
    cout << "Gibbs sampling iteration: " << i << endl;
    for (size_t j = 0; j < operators.size(); ++j) {
      cout << "Sampling with operator " << operators[j]->name() << endl;
      operators[j]->doIteration(sample,*options);
      for (size_t k = 0; k < collectors.size(); ++k) {
        collectors[k]->collect(sample);
      }
    }
  }
  
  
  for (size_t i = 0; i < operators.size(); ++i) {
    delete operators[i];
  }
  for (size_t i = 0; i < collectors.size(); ++i) {
    delete collectors[i];
  }

}

void PrintSampleCollector::collect(Sample& sample)  {
      cout << "Collected a sample" << endl;
      cout << *(sample.GetSampleHypothesis()) << endl;
    }

}
