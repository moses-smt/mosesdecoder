#ifndef moses_SpanLengthEstimator_h
#define moses_SpanLengthEstimator_h

#include <vector>
#include <map>

namespace Moses
{

class SpanLengthEstimator
{
public:
  typedef std::map<unsigned, float> TLengthToScoreMap;
  
  void AddSourceSpanScore(unsigned sourceSpanLength, float score);
  void AddTargetSpanScore(unsigned targetSpanLength, float score);
  float GetScoreBySpanLengths(unsigned sourceSpanLength, unsigned targetSpanLength) const;
  
private:
  static float FetchScoreFromMap(const TLengthToScoreMap& lengthToScoreMap, unsigned spanLength);
  
  TLengthToScoreMap m_sourceScores;
  TLengthToScoreMap m_targetScores;
};

}//namespace
#endif
