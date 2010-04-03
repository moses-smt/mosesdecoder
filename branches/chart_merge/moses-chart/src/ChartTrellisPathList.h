
#pragma once

#include <vector>

namespace MosesChart
{
class TrellisPath;

class TrellisPathList
{
protected:
	typedef std::vector<const TrellisPath*> CollType;
	 CollType m_collection;

public:
	// iters
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	
	iterator begin() { return m_collection.begin(); }
	iterator end() { return m_collection.end(); }
	const_iterator begin() const { return m_collection.begin(); }
	const_iterator end() const { return m_collection.end(); }
	void clear() { m_collection.clear(); }

	virtual ~TrellisPathList();

	size_t GetSize() const
	{ return m_collection.size(); }

	//! add a new entry into collection
	void Add(TrellisPath *trellisPath)
	{
		m_collection.push_back(trellisPath);
	}
};

} // namespace

