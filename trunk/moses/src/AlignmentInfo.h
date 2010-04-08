// $Id: AlignmentInfo.h 3048 2010-04-05 17:25:26Z hieuhoang1972 $
#pragma once

#include <list>
#include <ostream>

namespace Moses
{

// Collection of alignment pairs, ordered by source index
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

	void AddAlignment(const std::list<std::pair<size_t,size_t> > &alignmentList);
};

};

