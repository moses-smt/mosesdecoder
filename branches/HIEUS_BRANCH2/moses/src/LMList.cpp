
#include "LMList.h"
#include "Phrase.h"
#include "ScoreColl.h"

using namespace std;

void LMList::CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreColl *ngramComponent) const
{ 
	const_iterator lmIter;
	for (lmIter = begin(); lmIter != end(); ++lmIter)
	{
		const LanguageModel &lm = **lmIter;
		const float weightLM = lm.GetWeight();

		float fullScore, nGramScore;

		lm.CalcScore(phrase, fullScore, nGramScore);

		#ifdef N_BEST
			size_t lmId = lm.GetId();
			ngramComponent->SetValue(lmId, nGramScore);
		#endif

		retFullScore   += fullScore * weightLM;
		retNGramScore	+= nGramScore * weightLM;
	}	
}
