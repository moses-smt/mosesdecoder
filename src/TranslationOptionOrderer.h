#ifndef TRANSLATIONOPTIONORDERER_H_
#define TRANSLATIONOPTIONORDERER_H_

#endif /*TRANSLATIONOPTIONORDERER_H_*/

#include "TranslationOption.h"

using namespace std;

class TranslationOptionOrderer
{
	public:
	
	bool operator()(TranslationOption* optA, TranslationOption* optB) const
	{
		float futureScoreA = optA->GetFutureScore();
		float futureScoreB = optB->GetFutureScore();
		return (futureScoreA >= futureScoreB);
	}

};
