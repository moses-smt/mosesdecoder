// $Id$
#pragma once

#include "TranslationOptionCollection.h"

class Sentence;
class LMList;

class TranslationOptionCollectionText : public TranslationOptionCollection {
protected:
	
	void ProcessUnknownWord(		size_t sourcePos
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty);
	
 public:
	TranslationOptionCollectionText(Sentence const& inputSentence);
	
};

