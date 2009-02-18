#pragma once

#include "ChartCell.h"
#include "../../moses/src/WordsRange.h"

namespace MosesChart
{

class ChartCellCollection
{
public:
	typedef std::map<Moses::WordsRange, ChartCell*> CollType;
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

protected:
	CollType m_hypoStackColl;

public:
	const_iterator begin() const { return m_hypoStackColl.begin(); }
	const_iterator end() const { return m_hypoStackColl.end(); }
	iterator begin() { return m_hypoStackColl.begin(); }
	iterator end() { return m_hypoStackColl.end(); }
	const_iterator find(const Moses::WordsRange &range) const
	{ return m_hypoStackColl.find(range); }

	void Set(const Moses::WordsRange &range, ChartCell *cell)
	{ 
		m_hypoStackColl[range] = cell;
	}
};

}

