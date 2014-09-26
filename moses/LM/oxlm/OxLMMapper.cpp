#include "moses/LM/oxlm/OxLMMapper.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses {

OXLMMapper::OXLMMapper(const boost::shared_ptr<oxlm::Vocabulary>& vocab)
    : vocab(vocab) {
  for (int i = 0; i < vocab->size(); ++i) {
    const string &str = vocab->convert(i);
    FactorCollection &fc = FactorCollection::Instance();
    const Moses::Factor *factor = fc.AddFactor(str, false);
    moses2lbl[factor] = i;
  }

  kUNKNOWN = vocab->convert("<unk>");
}

int OXLMMapper::convert(const Moses::Factor* factor) const {
	Coll::const_iterator iter = moses2lbl.find(factor);
	if (iter == moses2lbl.end()) {
		return kUNKNOWN;
	} else {
		return iter->second;
	}
}

void OXLMMapper::convert(
    const std::vector<const Word*>& contextFactor,
    std::vector<int>& ids,
    int& word) const {
	for (size_t i = 0; i < contextFactor.size() - 1; ++i) {
		const Moses::Factor *factor = contextFactor[i]->GetFactor(0);
    ids.push_back(convert(factor));
	}
	std::reverse(ids.begin(), ids.end());

	const Moses::Factor *factor = contextFactor.back()->GetFactor(0);
	word = convert(factor);
}

} // namespace Moses
