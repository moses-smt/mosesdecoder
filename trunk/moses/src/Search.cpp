
#include "SearchCubePruning.h"
#include "SearchNormal.h"
#include "UserMessage.h"

namespace Moses
{
Search::Search()
{
//	long sentenceID = m_source.GetTranslationId();
//	m_constraint = staticData.GetConstrainingPhrase(sentenceID);
}

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
 
}


