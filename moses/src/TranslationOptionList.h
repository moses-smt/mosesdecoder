#pragma once

#include <vector>
#include <map>
#include "Phrase.h"

class TranslationOption;

class TranslationOptionList
{
	friend std::ostream& operator<<(std::ostream& out, const TranslationOptionList& obj);

protected:
	typedef std::vector<TranslationOption*> CollType;
	CollType m_coll;
	size_t m_trainingCount;

	typedef std::map<Phrase, size_t> UniqueTargetPhrase;
	UniqueTargetPhrase m_uniqueTargetPhrase;
		// unique pharse -> index to m_coll

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	TranslationOptionList()
		:m_trainingCount(0)
	{}
	TranslationOptionList(const TranslationOptionList &copy);
	~TranslationOptionList();

	size_t GetSize() const
	{ return m_coll.size(); }
	size_t GetTrainingCount() const 
	{ return m_trainingCount; }

	void Add(TranslationOption &transOpt);
	void Prune(size_t maxNoTransOptPerCoverage);
};

