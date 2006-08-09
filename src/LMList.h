
#pragma once

#include <list>
#include "LanguageModel.h"

class Phrase;
class ScoreColl;
class ScoreComponentCollection2;

class LMList : public std::list < LanguageModel* >	
{
public:
	void CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreComponentCollection2* breakdown) const;

};
