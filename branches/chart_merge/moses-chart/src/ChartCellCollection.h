#pragma once

#include "ChartCell.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/CellCollection.h"

namespace Moses
{
class InputType;
}

namespace MosesChart
{
class Manager;
	
class ChartCellCollection : public Moses::CellCollection
{
public:
	typedef std::vector<ChartCell*> InnerCollType;
	typedef std::vector<InnerCollType> OuterCollType;

protected:
	OuterCollType m_hypoStackColl;

public:
	ChartCellCollection(const Moses::InputType &input, Manager &manager);
	~ChartCellCollection();

	ChartCell &Get(const Moses::WordsRange &coverage)
	{
		return *m_hypoStackColl[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
	}
	const ChartCell &Get(const Moses::WordsRange &coverage) const
	{
		return *m_hypoStackColl[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
	}
	
	const std::vector<Moses::Word> &GetHeadwords(const Moses::WordsRange &coverage) const
	{
		const ChartCell &cell = Get(coverage);
		return cell.GetHeadwords();
	}
};

}

