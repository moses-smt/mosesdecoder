
#pragma once

#include <list>

namespace Moses
{

class AlignmentInfo
{
protected:
	std::list<std::pair<size_t,size_t> > m_collection;

public:
	void AddAlignment(const std::list<std::pair<size_t,size_t> > &alignmentList)
	{ m_collection = alignmentList; }
	void AddAlignment(const std::pair<size_t,size_t> &alignmentPair)
	{ m_collection.push_back(alignmentPair); }
};

};

