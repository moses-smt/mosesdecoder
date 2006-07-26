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
ProcessUnknownWord(		size_t sourcePos
											, int dropUnknown
											, FactorCollection &factorCollection
											, float weightWordPenalty) 
{
	ConfusionNet const& source=dynamic_cast<ConfusionNet const&>(m_source);

	ConfusionNet::Column const& coll=source.GetColumn(sourcePos);
	for(ConfusionNet::Column::const_iterator i=coll.begin();i!=coll.end();++i)
		ProcessOneUnknownWord(i->first.GetFactorArray(),sourcePos,dropUnknown,factorCollection,weightWordPenalty);
		
}

