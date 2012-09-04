#ifndef moses_SpanLengthEstimator_h
#define moses_SpanLengthEstimator_h

#include <vector>
#include <map>

namespace Moses
{

class SpanLengthEstimator
{
    private :
    std::vector< std::map< unsigned, float> > m_sourceScores;
    std::vector< std::map< unsigned, float> > m_targetScores;

    public :
    //SpanLengthEstimator();

    void AddSourceSpanProbas(std::map< unsigned, float> sourceProbas);
    void AddTargetSpanProbas(std::map< unsigned, float> targetProbas);

    float GetSourceLengthProbas(unsigned nonTerminal, unsigned spanLength) const;
    float GetTargetLengthProbas(unsigned nonTerminal, unsigned spanLenth) const;
};

}//namespace
#endif
