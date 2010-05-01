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
	size_t m_minInd, m_maxInd;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	//! iterators
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	size_t size() const { return m_coll.size(); }

	LMList()
	:m_maxNGramOrder(0)
	,m_minInd(std::numeric_limits<size_t>::max())
	,m_maxInd(0)
	{}
	~LMList();
	
	void CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreComponentCollection* breakdown) const;

	void CalcAllLMScores(const Phrase &phrase
								 , ScoreComponentCollection &nGramOnly
								 , ScoreComponentCollection *beginningBitsOnly) const ;
	
	void Add(LanguageModel *lm);

	size_t GetMaxNGramOrder() const
	{ return m_maxNGramOrder; }
	size_t GetMinIndex() const
	{ return m_minInd; }
	size_t GetMaxIndex() const
	{ return m_maxInd; }
	
};

}
#endif
