#include "moses/LM/oxlm/OxLMParallelMapper.h"

#include "lbl/parallel_vocabulary.h"

#include "moses/FactorCollection.h"

using namespace std;

namespace Moses {

OxLMParallelMapper::OxLMParallelMapper(
    const boost::shared_ptr<oxlm::Vocabulary>& vocab)
    : OxLMMapper(vocab) {
  boost::shared_ptr<oxlm::ParallelVocabulary> parallel_vocab =
      dynamic_pointer_cast<oxlm::ParallelVocabulary>(vocab);
  assert(parallel_vocab != nullptr);

  for (int i = 0; i < parallel_vocab->sourceSize(); ++i) {
    string word = parallel_vocab->convertSource(i);
    FactorCollection& fc = FactorCollection::Instance();
    const Moses::Factor* factor = fc.AddFactor(word, false);
    moses2SourceOxlm[factor] = i;
  }
}

int OxLMParallelMapper::convertSource(const Moses::Factor* factor) const {
  Coll::const_iterator iter = moses2SourceOxlm.find(factor);
  return iter == moses2SourceOxlm.end() ? kUNKNOWN : iter->second;
}

} // namespace Moses
