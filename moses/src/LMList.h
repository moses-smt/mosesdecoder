#ifndef moses_LMList_h
#define moses_LMList_h

#include <list>
#include "LM/Base.h"

namespace Moses
{

class Phrase;
class ScoreColl;
class ScoreComponentCollection;

//! List of language models and function to calc scores from each LM, given a phrase
class LMList
{
protected:
  typedef std::list < LanguageModel* > CollType;
  CollType m_coll;

  size_t m_minInd, m_maxInd;

public:
  typedef CollType::iterator iterator;
  typedef CollType::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const {
    return m_coll.begin();
  }
  const_iterator end() const {
    return m_coll.end();
  }
  size_t size() const {
    return m_coll.size();
  }

  LMList()
    :m_minInd(std::numeric_limits<size_t>::max())
    ,m_maxInd(0)
  {}
  void CleanUp();
  ~LMList();

  void CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, float &retOOVScore,  ScoreComponentCollection* breakdown) const;
  void InitializeBeforeSentenceProcessing() {
    std::list<LanguageModel*>::iterator lm_iter;
    for (lm_iter = m_coll.begin();
         lm_iter != m_coll.end();
         ++lm_iter) {
        (*lm_iter)->InitializeBeforeSentenceProcessing();
    }
  }
  void CleanUpAfterSentenceProcessing() {
    std::list<LanguageModel*>::iterator lm_iter;
    for (lm_iter = m_coll.begin();
         lm_iter != m_coll.end();
         ++lm_iter) {
        (*lm_iter)->CleanUpAfterSentenceProcessing();
    }
  }

  void Add(LanguageModel *lm);

  size_t GetMinIndex() const {
    return m_minInd;
  }
  size_t GetMaxIndex() const {
    return m_maxInd;
  }

};

}
#endif
