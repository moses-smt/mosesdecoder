// $Id$
#pragma once

#include "TranslationOptionCollection.h"

class Sentence;
class LMList;

class TranslationOptionCollectionText : public TranslationOptionCollection {
protected:
	
	void ProcessInitialTranslation(const DecodeStep &decodeStep
															, FactorCollection &factorCollection
															, float weightWordPenalty
															, int dropUnknown
															, size_t verboseLevel
															, PartialTranslOptColl &outputPartialTranslOptColl);
	void ProcessUnknownWord(		size_t sourcePos
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty);
	
 public:
	TranslationOptionCollectionText(Sentence const& inputSentence);
	
};

