// $Id$
#include "TranslationOptionCollectionConfusionNet.h"
#include "ConfusionNet.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"

TranslationOptionCollectionConfusionNet::
TranslationOptionCollectionConfusionNet(const ConfusionNet &input) 
	: TranslationOptionCollection(input) {}

int TranslationOptionCollectionConfusionNet::
HandleUnkownWord(PhraseDictionaryBase&,size_t,FactorCollection&,const LMList &,
								 bool,float) 
{
	return 0;
}
