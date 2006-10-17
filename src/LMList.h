
#pragma once

#include <list>
#include "LanguageModel.h"

class Phrase;
class ScoreColl;
class ScoreComponentCollection;

class LMList : public std::list < LanguageModel* >	
{
public:
	void CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreComponentCollection* breakdown) const;

};
