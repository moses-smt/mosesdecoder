#include "SpanLengthPhraseProperty.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
SpanLengthPhraseProperty::SpanLengthPhraseProperty()
{
}

void SpanLengthPhraseProperty::ProcessValue(const std::string &value)
{
  vector<string> toks;
  Tokenize(toks, value);

  set< vector<string> > indices;

  for (size_t i = 0; i < toks.size(); ++i) {
    const string &span = toks[i];

    // is it a ntIndex,sourceSpan,targetSpan  or count ?
    vector<string> toks;
    Tokenize<string>(toks, span, ",");
    UTIL_THROW_IF2(toks.size() != 1 && toks.size() != 3, "Incorrect format for SpanLength: " << span);

    if (toks.size() == 1) {
      float count = Scan<float>(toks[0]);
      Populate(indices, count);

      indices.clear();
    } else {
      indices.insert(toks);
    }
  }

  // totals
  CalcTotals(m_source);
  CalcTotals(m_target);
}

void SpanLengthPhraseProperty::Populate(const set< vector<string> > &indices, float count)
{
  set< vector<string> >::const_iterator iter;
  for (iter = indices.begin(); iter != indices.end(); ++iter) {
    const vector<string> &toksStr = *iter;
    vector<size_t> toks = Scan<size_t>(toksStr);
    UTIL_THROW_IF2(toks.size() != 3, "Incorrect format for SpanLength. Size is " << toks.size());

    Populate(toks, count);
  }
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

  Map &sourceMap = m_source[ntInd].first;
  Map &targetMap = m_target[ntInd].first;
  Populate(sourceMap, sourceLength, count);
  Populate(targetMap, targetLength, count);
}

void SpanLengthPhraseProperty::Populate(Map &map, size_t span, float count)
{
  Map::iterator iter;
  iter = map.find(span);
  if (iter != map.end()) {
    float &value = iter->second;
    value += count;
  } else {
    map[span] = count;
  }
}

void SpanLengthPhraseProperty::CalcTotals(Vec &vec)
{
  for (size_t i = 0; i < vec.size(); ++i) {
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

  if (map.size() == 0) {
    // should this ever be reached? there shouldn't be any span length proprty so FF shouldn't call this
    return 1.0f;
  }

  Map::const_iterator iter = map.find(sourceWidth);
  if (iter == map.end()) {
    count = 0;
  } else {
    count = iter->second;
  }
  count += smoothing;

  float total = data.second + smoothing * (float) map.size();
  float ret = count / total;
  return ret;
}

}
