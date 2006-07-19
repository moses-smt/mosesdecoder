// $Id$
#ifndef TRANSLATIONOPTIONCOLLECTIONCONFUSIONNET_H_
#define TRANSLATIONOPTIONCOLLECTIONCONFUSIONNET_H_
#include "TranslationOptionCollection.h"

class ConfusionNet;

class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection {
	const ConfusionNet &m_inputCN;
 public:
	TranslationOptionCollectionConfusionNet(const ConfusionNet &input);

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
