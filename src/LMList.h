
#pragma once

#include <list>
#include "LanguageModelSingleFactor.h"

class Phrase;
class ScoreColl;
class ScoreComponentCollection2;

class LMList : public std::list < LanguageModelSingleFactor* >	
{
public:
	void CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreComponentCollection2* breakdown) const;

};
