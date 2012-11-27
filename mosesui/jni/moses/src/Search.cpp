
#include "Manager.h"
#include "SearchCubePruning.h"
#include "SearchNormal.h"
#include "UserMessage.h"

namespace Moses
{


Search *Search::CreateSearch(Manager& manager, const InputType &source, 
                             SearchAlgorithm searchAlgorithm, const TranslationOptionCollection &transOptColl)
{
	switch(searchAlgorithm)
	{
		case Normal:		
			return new SearchNormal(manager,source, transOptColl);
		case CubePruning:
			return new SearchCubePruning(manager, source, transOptColl);
		case CubeGrowing:
			return NULL;
		default:
			UserMessage::Add("ERROR: search. Aborting\n");
			abort();
			return NULL;
	}

}
 
}


