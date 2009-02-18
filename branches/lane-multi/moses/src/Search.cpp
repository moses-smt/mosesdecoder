
#include "SearchCubePruning.h"
#include "SearchNormal.h"
#include "SearchMulti.h"
#include "UserMessage.h"

namespace Moses
{
Search::Search()
{
//	long sentenceID = m_source.GetTranslationId();
//	m_constraint = staticData.GetConstrainingPhrase(sentenceID);
}

//Search *Search::CreateSearch(const InputType &source, SearchAlgorithm searchAlgorithm, const TranslationOptionCollection &transOptColl)
//{
//	switch(searchAlgorithm)
//	{
//		case Normal:		
//			return new SearchNormal(source, transOptColl);
//		case CubePruning:
//			return new SearchCubePruning(source, transOptColl);
//		case CubeGrowing:
//			return NULL;
//		default:
//			UserMessage::Add("ERROR: search. Aborting\n");
//			abort();
//			return NULL;
//	}
//
//}
 
Search *Search::CreateSearch(std::vector< InputType const* > *sources, SearchAlgorithm searchAlgorithm, std::vector< TranslationOptionCollection* > *transOptColls)	
{
	switch(searchAlgorithm)
	{
		case Normal:		
			return new SearchNormal(*((*sources)[0]), *((*transOptColls)[0]));
		case CubePruning:
			return new SearchCubePruning(*((*sources)[0]), *((*transOptColls)[0]));
		case CubeGrowing:
			return NULL;
		case Multi:
			return new SearchMulti(*((*sources)[0]), *((*sources)[1]), *((*transOptColls)[0]), *((*transOptColls)[1]));
		default:
			UserMessage::Add("ERROR: search. Aborting\n");
			abort();
			return NULL;
	}
}
	
}


