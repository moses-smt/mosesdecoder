#include "Gibbler.h"

#include "Hypothesis.h"
#include "TranslationOptionCollection.h"

using namespace std;

namespace Moses {

Sample::Sample(Hypothesis* target_head) {
  
  source_size = target_head->GetWordsBitmap().GetSize();
  
  std::map<size_t, Hypothesis*> source_order;
  this->target_head = target_head;
  Hypothesis* next = NULL;

  for (Hypothesis* h = target_head; h->GetId() > 0; h = const_cast<Hypothesis*>(h->GetPrevHypo())) {
    size_t startPos = h->GetCurrSourceWordsRange().GetStartPos();
    size_t endPos = h->GetCurrSourceWordsRange().GetEndPos();

    for (size_t i = startPos; i <= endPos; i++) {
      sourceIndexedHyps[i] = h; 
    } 
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
  std::map<size_t, Hypothesis*>::iterator it;
  it = sourceIndexedHyps.find(i);
  assert(it != sourceIndexedHyps.end());
  return it->second;
}
  
void Sampler::Run(Hypothesis* starting, const TranslationOptionCollection* options) {
  size_t iterations = 5;
  vector<GibbsOperator*> operators;
  Sample sample(starting);
  SampleCollector* collector = new PrintSampleCollector();
  operators.push_back(new MergeSplitOperator());
  for (size_t i = 0; i < iterations; ++i) {
    cout << "Gibbs sampling iteration: " << i << endl;
    for (size_t j = 0; j < operators.size(); ++j) {
      cout << "Sampling with operator " << operators[j]->name() << endl;
      operators[j]->doIteration(sample,*options,*collector);
    }
  }
  
  
  for (size_t i = 0; i < operators.size(); ++i) {
    delete operators[i];
  }
  delete collector;

}

void PrintSampleCollector::collect(Sample& sample)  {
      cout << "Collected a sample" << endl;
    }

}
