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
	{}
	
	void CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, float &retOOVScore, ScoreComponentCollection* breakdown) const;
	void Add(LanguageModel *lm);


};

}
#endif
