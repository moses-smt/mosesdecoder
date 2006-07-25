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

void TranslationOptionCollectionConfusionNet::CreateTranslationOptions(
													const std::list < DecodeStep > &decodeStepList
												 , const LMList &allLM
												 , FactorCollection &factorCollection
												 , float weightWordPenalty
												 , bool dropUnknown
												 , size_t verboseLevel) 
{
#if 0
	ConfusionNet const& source(dynamic_cast<ConfusionNet&>(m_source));
	assert(dynamic_cast<PhraseDictionaryTreeAdaptor const*>(decodeStepList.front().GetDictionaryPtr()));
	PhraseDictionaryTreeAdaptor const& pdict
		=dynamic_cast<PhraseDictionaryTreeAdaptor const&>(*decodeStepList.front().GetDictionaryPtr());


	std::vector<State> stack;
	for(size_t i=0;i<src.GetSize();++i) stack.push_back(State(i,i,data.GetRoot()));



#else
//	TranslationOptionCollection::CreateTranslationOptions(decodeStepList,allLM,factorCollection,weightWordPenalty,dropUnknown,verboseLevel);
#endif
}
