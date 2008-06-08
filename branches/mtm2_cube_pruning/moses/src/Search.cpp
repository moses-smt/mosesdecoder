
#include "SearchCubePruning.h"
#include "SearchNormal.h"
#include "UserMessage.h"

Search *Search::CreateSearch(const InputType &source, SearchAlgorithm searchAlgorithm)
{
	switch(searchAlgorithm)
	{
		case Normal:		
			return new SearchNormal(source);
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

