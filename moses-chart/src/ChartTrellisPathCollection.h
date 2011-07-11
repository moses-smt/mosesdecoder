#pragma once

#include <set>
#include "ChartTrellisPath.h"

namespace MosesChart
{

class TrellisPath;

struct CompareTrellisPathCollection
{
	bool operator()(const TrellisPath* pathA, const TrellisPath* pathB) const
	{
		return (pathA->GetTotalScore() > pathB->GetTotalScore());
	}
};

class TrellisPathCollection
{
protected:
	typedef std::multiset<TrellisPath*, CompareTrellisPathCollection> CollectionType;
	CollectionType m_collection;

public:
	typedef CollectionType::iterator iterator;
	typedef CollectionType::const_iterator const_iterator;
	
	iterator begin() { return m_collection.begin(); }
	iterator end() { return m_collection.end(); }
	const_iterator begin() const { return m_collection.begin(); }
	const_iterator end() const { return m_collection.end(); }
	
	~TrellisPathCollection();

	size_t GetSize() const
	{ return m_collection.size(); }

	void Add(TrellisPath *path);
	void Prune(size_t newSize);

	TrellisPath *pop()
	{
		TrellisPath *top = *m_collection.begin();

		// Detach
		m_collection.erase(m_collection.begin());
		return top;
	}
	
	void Clear()
	{
		m_collection.clear();
	}
};


}

