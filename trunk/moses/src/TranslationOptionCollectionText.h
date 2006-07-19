// $Id$
#ifndef TRANSLATIONOPTIONCOLLECTIONTEXT_H_
#define TRANSLATIONOPTIONCOLLECTIONTEXT_H_
#include "TranslationOptionCollection.h"
#include "Sentence.h"

class TranslationOptionCollectionText : public TranslationOptionCollection {
	Sentence const& m_inputSentence;
 public:
	TranslationOptionCollectionText(Sentence const& inputSentence);

	void CreateTranslationOptions(const std::list < DecodeStep > &decodeStepList,
																const LMList &languageModels,
																const LMList &allLM,
																FactorCollection &factorCollection,
																float weightWordPenalty,
																bool dropUnknown,
																size_t verboseLevel);

	size_t GetSourceSize() const;
};
#endif
