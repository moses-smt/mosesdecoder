#ifndef moses_ProcessedNonTerminals_h
#define moses_ProcessedNonTerminals_h

#include <map>
#include <vector>
#include <set>
#include <Word.h>

#include "ChartHypothesisMBOT.h"
//#include "ChartHypothesis.h"

/**
 To Keep track of processed non terminals when creating output phrase
 */

namespace Moses
{
//class ChartHypothesis;
class ChartHypothesisMBOT;

class ProcessedNonTerminals
{

    public :
    //std::vector<MBOTHypoInfo*> m_mbotInfo;

    std::vector<const ChartHypothesisMBOT*> m_hypo;
    //std::vector<WordsRange> m_prev_range;
    int m_recnumber;
    int m_numTerms;
    std::map<size_t,int> m_status;
   //~ProcessedNonTerminals()
    //{
      //  RemoveAllInColl(m_mbotInfo);
    //}

    ProcessedNonTerminals()
    :m_recnumber(0){
       //BEWARE : Highest number of hypos hard coded
        m_hypo.reserve(500);
        m_numTerms = 0;
    }

    int GetTermNum()
    {
        return m_numTerms;
    }

    void IncrementTermNum()
    {
        //std::cout << "INCREMTENTING TERM NUMBER 1: " << m_numTerms << std::endl;
        m_numTerms++;
    }

    void ResetTermNum()
    {
        m_numTerms = 0;
    }

    void AddStatus(size_t hypoId, int stat)
    {
        //std::cout << "NT : Adding Status : " << hypoId << " : " << stat << std::endl;
        std::pair<std::map<size_t,int>::iterator,bool> ret;
        //Only add status when there is no status already
        ret=m_status.insert( std::pair<size_t,int>(hypoId,stat) );
        if (ret.second==false)
        {
           //std::cout << "NT : hypo already inside" << std::endl;

        }
        //std::cout << "NT : Added Status : " << hypoId << " : " << m_status.find(hypoId)->second << std::endl;
    }

    int GetStatus(size_t hypoId)
    {
        //std::cout << "GETTING STATUS : " << hypoId << std::endl;
        std::map<size_t,int>:: iterator itr_stat;
        //std::cout << "FOUND STATUS : " << m_status.find(hypoId)->second << std::endl;
        return m_status.find(hypoId)->second;
    }

    void IncrementStatus(size_t hypoId)
    {
        //std::cout << "BEFORE INCREMETING STATUS : " << std::endl;
        //std::cout << "BEFORE INCREMETING STATUS : " << m_status.find(hypoId)->second << " : ID " << hypoId << std::endl;
        m_status.find(hypoId)->second++;
        //std::cout << "AFTER INCREMETING STATUS : " << m_status.find(hypoId)->second << std::endl;
    }

    void AddHypothesis(const ChartHypothesisMBOT *current)
    {
        //std::cout << "Adding hypothesis : " << current <<  "at : " << m_recnumber << "Size : " << m_hypo.size() << std::endl;
        m_hypo[m_recnumber] = current;
    }

    const ChartHypothesisMBOT *GetHypothesis()
    {
        return m_hypo[m_recnumber];
    }

    const ChartHypothesisMBOT *GetHypothesis(size_t recNum)
    {
        return m_hypo[recNum];
    }

    int GetRecNumber()
    {
        return m_recnumber;
    }

    void Reset()
    {
        m_recnumber = 0;
        RemoveAllInColl(m_hypo);
        //m_prev_range.clear();
        m_status.clear();
    }

    void IncrementRec()
    {
        m_recnumber++;
        //std::cout << "AFTER INCREMENTING RECURSION : " << m_recnumber << std::endl;
    }

    void DecrementRec()
    {
        m_recnumber--;
        //std::cout << "AFTER DECREMENTING RECURSION" << m_recnumber << std::endl;
    }

    };
}
#endif
