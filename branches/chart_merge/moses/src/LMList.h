#ifndef moses_LMList_h
#define moses_LMList_h

#include <list>
#include "LanguageModel.h"

namespace Moses
{

class Phrase;
class ScoreColl;
class ScoreComponentCollection;

//! List of language models
class LMList
{
protected:
	typedef std::list < LanguageModel* > CollType;
	CollType m_coll; 
	
	size_t m_maxNGramOrder;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	//! iterators
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	size_t size() const { return m_coll.size(); }

	LMList()
	:m_maxNGramOrder(0)
	{}
	~LMList();
	
	void CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreComponentCollection* breakdown) const;

	void Add(LanguageModel *lm);

	size_t GetMaxNGramOrder() const
	{ return m_maxNGramOrder; }

};

}
#endif
