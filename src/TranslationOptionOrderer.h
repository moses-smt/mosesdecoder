#ifndef TRANSLATIONOPTIONORDERER_H_
#define TRANSLATIONOPTIONORDERER_H_

#endif /*TRANSLATIONOPTIONORDERER_H_*/

#include "TranslationOption.h"

class TranslationOptionOrderer
{
	public:
	
	bool operator()(TranslationOption* optA, TranslationOption* optB) const
	{
		ScoreComponentCollection scoreBreakdownA = optA->ReturnScoreBreakdown();
		ScoreComponentCollection scoreBreakdownB = optB->ReturnScoreBreakdown();
		vector<float> scoresA = scoreBreakdownA.ReturnScores();
		vector<float> scoresB = scoreBreakdownB.ReturnScores();
		
		vector<float>::iterator scoresA_iter;
		float combinedScoreA = 0.0;
		for(scoresA_iter = scoresA.begin(); scoresA_iter != scoresA.end(); ++scoresA_iter)
		{
			float f = *scoresA_iter;
			// combine the scores of scoreBreakdown by just summing them up
			// TODO: weighted combination
			combinedScoreA += f;
		}
		
		vector<float>::iterator scoresB_iter;
		float combinedScoreB = 0.0;
		for(scoresB_iter = scoresB.begin(); scoresB_iter != scoresB.end(); ++scoresB_iter)
		{
			float f = *scoresB_iter;
			// combine the scores of scoreBreakdown by just summing them up
			// TODO: weighted combination
			combinedScoreB += f;
		}
		
		return (combinedScoreA >= combinedScoreB);
	}

};
