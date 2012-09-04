#ifndef moses_SpanLengthEstimator_h
#define moses_SpanLengthEstimator_h

#include <vector>
#include <map>

namespace Moses
{

class SpanLengthEstimator
{
    private :
    std::vector< std::map< std::size_t, float> > m_sourceScores;
    std::vector< std::map< std::size_t, float> > m_targetScores;

    public :
    //SpanLengthEstimator();

    void AddSourceSpanProbas(std::map< std::size_t, float> sourceProbas);
    void AddTargetSpanProbas(std::map< std::size_t, float> targetProbas);

    float GetSourceLengthProbas(std::size_t nonTerminal, std::size_t spanLength) const;
    float GetTargetLengthProbas(std::size_t nonTerminal, std::size_t spanLenth) const;
};

}//namespace
#endif
