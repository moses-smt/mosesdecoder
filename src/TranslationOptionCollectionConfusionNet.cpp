// $Id$
#include "TranslationOptionCollectionConfusionNet.h"
#include "ConfusionNet.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"

TranslationOptionCollectionConfusionNet::TranslationOptionCollectionConfusionNet(const ConfusionNet &input) 
	: TranslationOptionCollection(input.GetSize()),m_inputCN(input) {}

size_t TranslationOptionCollectionConfusionNet::GetSourceSize() const 
{
	return m_inputCN.GetSize();
}
void TranslationOptionCollectionConfusionNet::
CreateTranslationOptions(const std::list < DecodeStep > &decodeStepList,
												 const LMList &languageModels,
												 const LMList &allLM,
												 FactorCollection &factorCollection,
												 float weightWordPenalty,
												 bool dropUnknown,
												 size_t verboseLevel)
{
}
