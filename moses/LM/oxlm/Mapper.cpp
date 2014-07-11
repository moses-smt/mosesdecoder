#include "Mapper.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{
OXLMMapper::OXLMMapper(const oxlm::Dict& dict) : dict(dict)
{
  for (int i = 0; i < dict.size(); ++i) {
	const string &str = dict.Convert(i);
	FactorCollection &fc = FactorCollection::Instance();
	const Moses::Factor *factor = fc.AddFactor(str, false);
	moses2lbl[factor] = i;

    //add(i, TD::Convert());
  }

  kUNKNOWN = this->dict.Convert("<unk>");
}

int OXLMMapper::convert(const Moses::Factor *factor) const
{
	Coll::const_iterator iter;
	iter = moses2lbl.find(factor);
	if (iter == moses2lbl.end()) {
		return kUNKNOWN;
	}
	else {
		int ret = iter->second;
		return ret;
	}
}

std::vector<int> OXLMMapper::convert(const Phrase &phrase) const
{
	size_t size = phrase.GetSize();
	vector<int> ret(size);

	for (size_t i = 0; i < size; ++i) {
		const Moses::Factor *factor = phrase.GetFactor(i, 0);
		int id = convert(factor);
		ret[i] = id;
	}
	return ret;
}

} // namespace

