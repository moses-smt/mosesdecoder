
#pragma once

#include <list>

namespace Moses
{

class AlignmentInfo
{
	friend std::ostream& operator<<(std::ostream&, const AlignmentInfo&);

protected:
	typedef std::list<std::pair<size_t,size_t> > CollType;
	CollType m_collection;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_collection.begin(); }
	const_iterator end() const { return m_collection.end(); }

	void AddAlignment(const std::list<std::pair<size_t,size_t> > &alignmentList)
	{ m_collection = alignmentList; }
	void AddAlignment(const std::pair<size_t,size_t> &alignmentPair)
	{ m_collection.push_back(alignmentPair); }
};

};

