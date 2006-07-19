
#include "LMList.h"
#include "Phrase.h"
#include "ScoreColl.h"

using namespace std;

void LMList::SetScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreColl *ngramComponent) const
{ 
	const_iterator lmIter;
	for (lmIter = begin(); lmIter != end(); ++lmIter)
	{
		const LanguageModel &lm = **lmIter;
		const float weightLM = lm.GetWeight();

		float fullScore, nGramScore;

		#ifdef N_BEST
				lm.CalcScore(phrase, fullScore, nGramScore, &ngramComponent);
		#else
		    // this is really, really ugly (a reference to an object at NULL
		    // is asking for trouble). TODO
				lm.CalcScore(phrase, fullScore, nGramScore, NULL);
		#endif

		retFullScore   += fullScore * weightLM;
		retNGramScore	+= nGramScore * weightLM;
	}	
}
