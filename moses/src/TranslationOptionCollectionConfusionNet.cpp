// $Id$
#include "TranslationOptionCollectionConfusionNet.h"
#include "ConfusionNet.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "LMList.h"

TranslationOptionCollectionConfusionNet::
TranslationOptionCollectionConfusionNet(const ConfusionNet &input) 
	: TranslationOptionCollection(input) {}

void TranslationOptionCollectionConfusionNet::
ProcessInitialTranslation(const DecodeStep &decodeStep
													, FactorCollection &factorCollection
													, float weightWordPenalty
													, int dropUnknown
													, size_t verboseLevel
													, PartialTranslOptColl &outputPartialTranslOptColl) 
{
}

void TranslationOptionCollectionConfusionNet::
ProcessUnknownWord(		size_t sourcePos
											, int dropUnknown
											, FactorCollection &factorCollection
											, float weightWordPenalty) 
{
}

