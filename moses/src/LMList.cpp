
#include "LMList.h"
#include "Phrase.h"
#include "ScoreComponentCollection.h"

using namespace std;

void LMList::CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreComponentCollection2* breakdown) const
{ 
	const_iterator lmIter;
	for (lmIter = begin(); lmIter != end(); ++lmIter)
	{
		const LanguageModel &lm = **lmIter;
		const float weightLM = lm.GetWeight();

		float fullScore, nGramScore;

		lm.CalcScore(phrase, fullScore, nGramScore);

		#ifdef N_BEST
			breakdown->Assign(&lm, nGramScore);  // I'm not sure why += doesn't work here- it should be 0.0 right?
		#endif

		retFullScore   += fullScore * weightLM;
		retNGramScore	+= nGramScore * weightLM;
	}	
}
