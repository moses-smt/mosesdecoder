
#pragma once

#include <string>
#include <map>
#include <vector>
#include "moses/PP/PhraseProperty.h"

namespace Moses
{

class SpanLengthPhraseProperty : public PhraseProperty
{
public:
	SpanLengthPhraseProperty(const std::string &value);

protected:
	// fractional counts
	typedef std::map<size_t, float> Map;
	typedef std::vector<Map> Vec;
	Vec m_source, m_target;

	void Populate(const std::string &span, float count);
	void Populate(const std::vector<size_t> &toks, float count);
};

} // namespace Moses

