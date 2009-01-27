#include "Gibbler.h"

#include "Hypothesis.h"
#include "TranslationOptionCollection.h"

using namespace std;

namespace Moses {

void Sampler::Run(Hypothesis* starting, const TranslationOptionCollection* options) {
  for (Hypothesis* h = starting; h; h = const_cast<Hypothesis*>(h->GetPrevHypo())) {
  }
}

}
