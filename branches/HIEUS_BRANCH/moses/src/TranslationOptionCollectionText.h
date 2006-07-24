// $Id$
#ifndef TRANSLATIONOPTIONCOLLECTIONTEXT_H_
#define TRANSLATIONOPTIONCOLLECTIONTEXT_H_
#include "TranslationOptionCollection.h"

class Sentence;

class TranslationOptionCollectionText : public TranslationOptionCollection {
 public:
	TranslationOptionCollectionText(Sentence const& inputSentence);

	int HandleUnkownWord(PhraseDictionaryBase& phraseDictionary,
											 size_t startPos,
											 FactorCollection &factorCollection,
											 const LMList &allLM,
											 bool dropUnknown,
											 float weightWordPenalty
											 ); 
};
#endif
