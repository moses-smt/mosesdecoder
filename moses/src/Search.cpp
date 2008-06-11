
#include "SearchCubePruning.h"
#include "SearchNormal.h"
#include "UserMessage.h"

Search *Search::CreateSearch(const InputType &source, SearchAlgorithm searchAlgorithm, const TranslationOptionCollection &transOptColl)
{
	switch(searchAlgorithm)
	{
		case Normal:		
			return new SearchNormal(source, transOptColl);
		case CubePruning:
			return new SearchCubePruning(source, transOptColl);
		case CubeGrowing:
			return NULL;
		default:
			UserMessage::Add("ERROR: search. Aborting\n");
			abort();
			return NULL;
	}

}

