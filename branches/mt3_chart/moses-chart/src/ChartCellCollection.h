#pragma once

#include "ChartCell.h"
#include "../../moses/src/WordsRange.h"

namespace MosesChart
{

class ChartCellSignature
{
protected:
	Moses::WordsRange m_coverage;

public:
	explicit ChartCellSignature(const Moses::WordsRange &coverage)
		:m_coverage(coverage)
	{}

	const Moses::WordsRange &GetCoverage() const
	{ return m_coverage; }

	//! transitive comparison used for adding objects into set
	inline bool operator<(const ChartCellSignature &compare) const
	{
		return m_coverage < compare.m_coverage;
	}

};

class ChartCellCollection
{
public:
	typedef std::map<ChartCellSignature, ChartCell*> CollType;
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

protected:
	CollType m_hypoStackColl;

public:
	const_iterator begin() const { return m_hypoStackColl.begin(); }
	const_iterator end() const { return m_hypoStackColl.end(); }
	iterator begin() { return m_hypoStackColl.begin(); }
	iterator end() { return m_hypoStackColl.end(); }
	const_iterator find(const ChartCellSignature &signature) const
	{ return m_hypoStackColl.find(signature); }

	const ChartCell *Get(const ChartCellSignature &signature) const;
	ChartCell *GetOrCreate(const ChartCellSignature &signature);
	
};

}

