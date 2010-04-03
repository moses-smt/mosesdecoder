
#include "ChartCellCollection.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/WordsRange.h"

namespace MosesChart
{
ChartCellCollection::ChartCellCollection(const Moses::InputType &input, Manager &manager)
:m_hypoStackColl(input.GetSize())
{
	size_t size = input.GetSize();
	for (size_t startPos = 0; startPos < size; ++startPos)
	{
		InnerCollType &inner = m_hypoStackColl[startPos];
		inner.resize(size - startPos);

		size_t ind = 0;
		for (size_t endPos = startPos ; endPos < size; ++endPos)
		{
			ChartCell *cell = new ChartCell(startPos, endPos, manager);
			inner[ind] = cell;
			++ind;
		}
	}
}

ChartCellCollection::~ChartCellCollection()
{
	OuterCollType::iterator iter;
	for (iter = m_hypoStackColl.begin(); iter != m_hypoStackColl.end(); ++iter)
	{
		InnerCollType &inner = *iter;
		Moses::RemoveAllInColl(inner);
	}
}

} // namespace

