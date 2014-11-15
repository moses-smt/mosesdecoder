#include "OxLMMapper.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses {

OxLMMapper::OxLMMapper(const boost::shared_ptr<oxlm::Vocabulary>& vocab) {
  for (int i = 0; i < vocab->size(); ++i) {
    const string &str = vocab->convert(i);
    FactorCollection &fc = FactorCollection::Instance();
    const Moses::Factor *factor = fc.AddFactor(str, false);
    moses2Oxlm[factor] = i;
  }

  kUNKNOWN = vocab->convert("<unk>");
}

int OxLMMapper::convert(const Moses::Factor *factor) const {
	Coll::const_iterator iter = moses2Oxlm.find(factor);
  return iter == moses2Oxlm.end() ? kUNKNOWN : iter->second;
}

void OxLMMapper::convert(
    const vector<const Word*> &contextFactor,
    vector<int> &ids, int &word) const {
  ids.clear();
	for (size_t i = 0; i < contextFactor.size() - 1; ++i) {
		const Moses::Factor *factor = contextFactor[i]->GetFactor(0);
    ids.push_back(convert(factor));
	}
	reverse(ids.begin(), ids.end());

	const Moses::Factor *factor = contextFactor.back()->GetFactor(0);
	word = convert(factor);
}

} // namespace Moses
