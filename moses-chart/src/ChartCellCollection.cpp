
#include "ChartCellCollection.h"

namespace MosesChart
{

const ChartCell *ChartCellCollection::Get(const ChartCellSignature &signature) const
{
	const_iterator iter = m_hypoStackColl.find(signature);
	assert(iter != m_hypoStackColl.end());
	return iter->second;
}

ChartCell *ChartCellCollection::GetOrCreate(const ChartCellSignature &signature)
{
	const_iterator iter = m_hypoStackColl.find(signature);
	if (iter == m_hypoStackColl.end())
	{
		ChartCell *cell = new ChartCell(signature.GetCoverage().GetStartPos()
																	, signature.GetCoverage().GetEndPos());

		m_hypoStackColl[signature] = cell;
		return cell;
	}
	else
	{
		return iter->second;
	}
}

}

