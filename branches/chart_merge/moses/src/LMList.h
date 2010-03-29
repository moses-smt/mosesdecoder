#ifndef moses_LMList_h
#define moses_LMList_h

#include <list>
#include "LanguageModel.h"

namespace Moses
{

class Phrase;
class ScoreColl;
class ScoreComponentCollection;

//! List of language models
class LMList : public std::list < LanguageModel* >	
{
public:
	void CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, ScoreComponentCollection* breakdown) const;

};

}
#endif
