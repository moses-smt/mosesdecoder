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
  m_source[ntInd][sourceLength] = count;
  m_target[ntInd][targetLength] = count;

}

}
