#include "moses/LM/oxlm/OxLMMapper.h"

#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{

OxLMMapper::OxLMMapper(
  const boost::shared_ptr<oxlm::Vocabulary>& vocab,
  bool pos_back_off,
  const FactorType& pos_factor_type)
  : posBackOff(pos_back_off), posFactorType(pos_factor_type)
{
  for (int i = 0; i < vocab->size(); ++i) {
    const string &str = vocab->convert(i);
    FactorCollection &fc = FactorCollection::Instance();
    const Moses::Factor *factor = fc.AddFactor(str, false);
    moses2Oxlm[factor] = i;
  }

  kUNKNOWN = vocab->convert("<unk>");
}

int OxLMMapper::convert(const Word& word) const
{
  const Moses::Factor* word_factor = word.GetFactor(0);
  Coll::const_iterator iter = moses2Oxlm.find(word_factor);
  if (posBackOff && iter == moses2Oxlm.end()) {
    const Moses::Factor* pos_factor = word.GetFactor(posFactorType);
    iter = moses2Oxlm.find(pos_factor);
  }

  return iter == moses2Oxlm.end() ? kUNKNOWN : iter->second;
}

void OxLMMapper::convert(
  const vector<const Word*>& contextFactor,
  vector<int> &ids, int &word) const
{
  ids.clear();
  for (size_t i = 0; i < contextFactor.size() - 1; ++i) {
    ids.push_back(convert(*contextFactor[i]));
  }
  std::reverse(ids.begin(), ids.end());

  word = convert(*contextFactor.back());
}

} // namespace Moses
