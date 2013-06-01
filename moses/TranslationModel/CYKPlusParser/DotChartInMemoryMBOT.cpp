//Fabienne Braune
//In memory format for l-MBOT rules

#include "DotChartInMemoryMBOT.h"

#include "moses/Util.h"

#include <algorithm>

namespace Moses
{

DottedRuleCollMBOT::~DottedRuleCollMBOT()
{
	std::cerr << "DELETING DOTTED RULE COLLECTION..." << std::endl;
#ifdef USE_BOOST_POOL
  // Do nothing.  DottedRule objects are stored in object pools owned by
  // the sentence-specific ChartRuleLookupManagers.
#else
  std::cerr << "DELETING STUFF IN COLLECTION..." << std::endl;
  std::for_each(m_mbotColl.begin(), m_mbotColl.end(),
                RemoveAllInColl<CollTypeMBOT::value_type>);
#endif
}

}
