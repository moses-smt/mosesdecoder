
#include "ChartTrellisPathList.h"
#include "ChartTrellisPath.h"
#include "../../moses/src/Util.h"

namespace MosesChart
{
TrellisPathList::~TrellisPathList()
{
	// clean up
	Moses::RemoveAllInColl(m_collection);
}

}

