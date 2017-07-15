
#pragma once

#include <string>
#include <set>
#include <map>
#include <vector>
#include "moses/PP/PhraseProperty.h"

namespace Moses
{

class SpanLengthPhraseProperty : public PhraseProperty
{
public:
  SpanLengthPhraseProperty();

  void ProcessValue(const std::string &value);

  float GetProb(size_t ntInd, size_t sourceWidth, float smoothing) const;
protected:
  // fractional counts
  typedef std::map<size_t, float> Map;
  typedef std::vector<std::pair<Map, float> > Vec;
  Vec m_source, m_target;

  void Populate(const std::set< std::vector<std::string> > &indices, float count);
  void Populate(const std::vector<size_t> &toks, float count);
  void Populate(Map &map, size_t span, float count);

  void CalcTotals(Vec &vec);
};

} // namespace Moses

