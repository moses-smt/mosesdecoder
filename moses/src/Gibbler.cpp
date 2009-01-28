#include "Gibbler.h"

#include "Hypothesis.h"
#include "TranslationOptionCollection.h"

using namespace std;

namespace Moses {

Sample::Sample(Hypothesis* target_head) {
  this->target_head = target_head;
  Hypothesis* next = NULL;
  for (Hypothesis* h = target_head; h; h = const_cast<Hypothesis*>(h->GetPrevHypo())) {
    this->target_tail = h;
    h->m_nextHypo = next;
    next = h;
  }
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
