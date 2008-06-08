
#include "SearchCubePruning.h"
#include "UserMessage.h"

Search *Search::CreateSearch(const InputType &source, SearchAlgorithm searchAlgorithm)
{
	switch(searchAlgorithm)
	{
		case Normal:		
			return NULL;
		case CubePruning:
			return new SearchCubePruning(source);
		case CubeGrowing:
			return NULL;
		default:
			UserMessage::Add("ERROR: search. Aborting\n");
			abort();
			return NULL;
	}

}

