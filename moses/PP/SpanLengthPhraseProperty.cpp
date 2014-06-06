#include <vector>
#include "SpanLengthPhraseProperty.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
SpanLengthPhraseProperty::SpanLengthPhraseProperty(const std::string &value)
: PhraseProperty(value)
{
  vector<string> toks;
  Tokenize(toks, value);

  for (size_t i = 0; i < toks.size(); i = i + 2) {
	  const string &span = toks[i];
	  float count = Scan<float>(toks[i + 1]);
	  Populate(span, count);
  }

  // totals
  CalcTotals(m_source);
  CalcTotals(m_target);
}

void SpanLengthPhraseProperty::Populate(const string &span, float count)
{
  vector<size_t> toks;
  Tokenize<size_t>(toks, span, ",");
  UTIL_THROW_IF2(toks.size() != 3, "Incorrect format for SpanLength: " << span);
  Populate(toks, count);
}

void SpanLengthPhraseProperty::Populate(const std::vector<size_t> &toks, float count)
{
  size_t ntInd = toks[0];
  size_t sourceLength = toks[1];
  size_t targetLength = toks[2];
  if (ntInd >=  m_source.size() ) {
	  m_source.resize(ntInd + 1);
	  m_target.resize(ntInd + 1);
  }
  m_source[ntInd].first[sourceLength] = count;
  m_target[ntInd].first[targetLength] = count;

}

void SpanLengthPhraseProperty::CalcTotals(Vec &vec)
{
	for (size_t i = 0; i < vec.size(); i = i + 2) {
		float total = 0;

		const Map &map = vec[i].first;
		Map::const_iterator iter;
		for (iter = map.begin(); iter != map.end(); ++iter) {
			float count = iter->second;
			total += count;
		}

		vec[i].second = total;
	}
}

float SpanLengthPhraseProperty::GetProb(size_t ntInd, size_t sourceWidth, float smoothing) const
{
	float count;

	const std::pair<Map, float> &data = m_source[ntInd];
	const Map &map = data.first;
	Map::const_iterator iter = map.find(sourceWidth);
	if (iter == map.end()) {
	  count = 0;
	}
	else {
      count = iter->second;
	}
	count += smoothing;

	float ret = count / (data.second + smoothing * map.size());
	return ret;
}

}
