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
}

}
