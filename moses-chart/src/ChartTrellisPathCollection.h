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

};


}

