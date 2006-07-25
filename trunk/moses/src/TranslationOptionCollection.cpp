#include "TranslationOptionCollection.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "Input.h"

using namespace std;

TranslationOptionCollection::TranslationOptionCollection(InputType const& src)
	: m_source(src)
	,m_futureScore(src.GetSize())
	,m_unknownWordPos(src.GetSize())
{
}

TranslationOptionCollection::~TranslationOptionCollection()
{
}

void TranslationOptionCollection::CalcFutureScore(size_t verboseLevel)
{
	// create future score matrix
	// for each span in the source phrase (denoted by start and end)
	for(size_t startPos = 0; startPos < m_source.GetSize() ; startPos++) 
	{
		for(size_t endPos = startPos; endPos < m_source.GetSize() ; endPos++) 
		{
			size_t length = endPos - startPos + 1;
			vector< float > score(length + 1);
			score[0] = 0;
			for(size_t currLength = 1 ; currLength <= length ; currLength++) 
			// initalize their future cost to -infinity
			{
				score[currLength] = - numeric_limits<float>::infinity();
			}

			for(size_t currLength = 0 ; currLength < length ; currLength++) 
			{
				// iterate over possible translations of this source subphrase and
				// keep track of the highest cost option
				TranslationOptionCollection::const_iterator iterTransOpt;
				for(iterTransOpt = begin() ; iterTransOpt != end() ; ++iterTransOpt)
				{
					const TranslationOption &transOpt = *iterTransOpt;
					size_t index = currLength + transOpt.GetSize();
					if (transOpt.GetStartPos() == currLength + startPos 
							&& transOpt.GetEndPos() <= endPos
							&& transOpt.GetFutureScore() + score[currLength] > score[index]) 
					{
						score[index] = transOpt.GetFutureScore() + score[currLength];
					}
				}
			}
			// record the highest cost option in the future cost table.
			m_futureScore.SetScore(startPos, endPos, score[length]);

			//print information about future cost table when verbose option is set

			if(verboseLevel > 0) 
			{		
				cout<<"future cost from "<<startPos<<" to "<<endPos<<" is "<<score[length]<<endl;
			}
		}
	}
}

