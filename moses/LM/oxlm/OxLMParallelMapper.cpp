#include "moses/LM/oxlm/OxLMParallelMapper.h"

#include "lbl/parallel_vocabulary.h"

#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{

OxLMParallelMapper::OxLMParallelMapper(
  const boost::shared_ptr<oxlm::Vocabulary>& vocab,
  bool pos_back_off,
  const FactorType& pos_factor_type)
  : OxLMMapper(vocab, pos_back_off, pos_factor_type)
{
  boost::shared_ptr<oxlm::ParallelVocabulary> parallel_vocab =
    dynamic_pointer_cast<oxlm::ParallelVocabulary>(vocab);
  assert(parallel_vocab != nullptr);

  for (int i = 0; i < parallel_vocab->sourceSize(); ++i) {
    string word = parallel_vocab->convertSource(i);
    FactorCollection& fc = FactorCollection::Instance();
    const Moses::Factor* factor = fc.AddFactor(word, false);
    moses2SourceOxlm[factor] = i;
  }

  kSOURCE_UNKNOWN = parallel_vocab->convertSource("<unk>");
}

int OxLMParallelMapper::convertSource(const Word& word) const
{
  const Moses::Factor* word_factor = word.GetFactor(0);
  Coll::const_iterator iter = moses2SourceOxlm.find(word_factor);
  if (posBackOff && iter == moses2SourceOxlm.end()) {
    const Moses::Factor* pos_factor = word.GetFactor(posFactorType);
    iter = moses2SourceOxlm.find(pos_factor);
  }
  return iter == moses2SourceOxlm.end() ? kSOURCE_UNKNOWN : iter->second;
}

} // namespace Moses
